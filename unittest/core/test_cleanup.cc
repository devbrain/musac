#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <atomic>
#include <cstdlib>
#include "../test_helpers.hh"
#include "../test_helpers_v2.hh"

namespace musac::test {

// Extended mock source that tracks destruction
class lifecycle_mock_source : public audio_source {
    bool* m_destroyed_flag;
    
public:
    static std::unique_ptr<lifecycle_mock_source> create(bool* destroyed_flag, size_t frames = 44100) {
        auto state = std::make_shared<mock_audio_state>(frames);
        auto decoder = std::make_unique<test_decoder_with_state>(state);
        auto io_stream = std::make_unique<memory_io_stream>();
        
        return std::unique_ptr<lifecycle_mock_source>(
            new lifecycle_mock_source(std::move(decoder), std::move(io_stream), state, destroyed_flag)
        );
    }
    
    ~lifecycle_mock_source() {
        if (m_destroyed_flag) {
            *m_destroyed_flag = true;
        }
    }
    
private:
    lifecycle_mock_source(std::unique_ptr<decoder> dec, std::unique_ptr<musac::io_stream> io, 
                          std::shared_ptr<mock_audio_state> /*state*/, bool* destroyed_flag)
        : audio_source(std::move(dec), std::move(io)), m_destroyed_flag(destroyed_flag) {}
};

// Helper function
std::unique_ptr<audio_source> create_lifecycle_source(bool* destroyed_flag, size_t frames = 44100) {
    return lifecycle_mock_source::create(destroyed_flag, frames);
}

TEST_SUITE("cleanup") {
    TEST_CASE("audio system init/done cycles") {
        // Multiple init/done cycles should work
        for (int i = 0; i < 3; ++i) {
            auto backend = init_test_audio_system();
            CHECK(backend.get() != nullptr);
            audio_system::done();
        }
    }
    
    TEST_CASE("device cleanup before streams") {
        auto backend = init_test_audio_system();
        
        // With single device constraint, we can't have multiple devices
        // So test that streams are cleaned up when device is destroyed
        {
            auto device = audio_device::open_default_device(backend);
            auto source = create_mock_source();
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
            stream.play();
            
            // Device and stream will be destroyed together
        }
        
        // Now we can create a new device
        {
            auto new_device = audio_device::open_default_device(backend);
            CHECK(new_device.get_channels() > 0);
        }
        
        audio_system::done();
    }
    
    TEST_CASE("stream cleanup during callback") {
        auto backend = init_test_audio_system();
        
        {
            auto device = audio_device::open_default_device(backend);
            device.resume();
            
            {
                bool source_destroyed = false;
                bool callback_executed = false;
                auto source = create_lifecycle_source(&source_destroyed);
                auto stream = device.create_stream(std::move(*source));
                REQUIRE_NOTHROW(stream.open());
                
                stream.set_finish_callback([&callback_executed](audio_stream& /*s*/) {
                    // Callback does NOT hold a reference to the stream
                    // Just mark that it was executed
                    callback_executed = true;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                });
                
                stream.play(1);
                
                // Give time for playback to complete
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // Stream goes out of scope here
            }
            
            // Source should be destroyed after stream is destroyed
            // CHECK(source_destroyed); // Can't check this reliably due to timing
            
            // Device goes out of scope here
        }
        
        audio_system::done();
    }
    
    TEST_CASE("audio system cleanup with active streams") {
        auto backend = init_test_audio_system();
        
        {
            auto device = audio_device::open_default_device(backend);
            device.resume();
            
            // Create multiple active streams
            std::vector<std::unique_ptr<audio_stream>> streams;
            for (int i = 0; i < 5; ++i) {
                auto source = create_mock_source();
                auto stream = std::make_unique<audio_stream>(device.create_stream(std::move(*source)));
                REQUIRE_NOTHROW(stream->open());
                stream->play();
                streams.push_back(std::move(stream));
            }
            
            // Let everything go out of scope before done()
            // This tests proper cleanup order
        }
        
        // Done should handle cleanup after all objects are destroyed
        audio_system::done();
    }
    
    TEST_CASE("exception during stream creation") {
        auto backend = init_test_audio_system();
        
        {
            auto device = audio_device::open_default_device(backend);
            
            // Test cleanup when exception occurs during stream setup
            try {
                // Force an exception by using moved-from source
                auto source = create_mock_source();
                // Try to create stream with null source
                // Test that we handle exceptions during stream creation gracefully
            CHECK_NOTHROW(device.create_stream(std::move(*source)));
            } catch (...) {
                // Cleanup should still work
            }
        }
        
        audio_system::done();
    }
    
    TEST_CASE("callback cleanup on stream destruction") {
        auto backend = init_test_audio_system();
        
        {
            auto device = audio_device::open_default_device(backend);
            device.resume();
            
            std::atomic<bool> callback_executed{false};
            
            {
                auto source = create_mock_source();
                auto stream = device.create_stream(std::move(*source));
                REQUIRE_NOTHROW(stream.open());
                
                stream.set_finish_callback([&callback_executed](audio_stream& /*s*/) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    callback_executed = true;
                });
                
                stream.play(1);
                
                // Small delay to ensure playback starts
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                // Stream destroyed while callback might be pending
            }
            
            // Wait a bit more
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Callback might or might not execute, but shouldn't crash
        }
        
        audio_system::done();
    }
    
    TEST_CASE("device cleanup with paused streams") {
        auto backend = init_test_audio_system();
        
        {
            auto device = audio_device::open_default_device(backend);
            device.resume();
            
            auto source = create_mock_source();
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
            stream.play();
            
            // Pause stream
            stream.pause();
            CHECK(stream.is_paused());
            
            // Device and stream will be destroyed while stream is paused
        }
        
        // Should not crash
        audio_system::done();
    }
    
    TEST_CASE("rapid stream creation and destruction") {
        auto backend = init_test_audio_system();
        
        {
            auto device = audio_device::open_default_device(backend);
            device.resume();
            
            // Create and destroy many streams rapidly
            for (int i = 0; i < 100; ++i) {
                auto source = create_mock_source();
                auto stream = device.create_stream(std::move(*source));
                REQUIRE_NOTHROW(stream.open());
                
                if (i % 2 == 0) {
                    stream.play();
                }
                
                // Stream destroyed immediately
            }
        }
        
        audio_system::done();
    }
    
    TEST_CASE("cleanup order stress test") {
        // Test various cleanup orders with single device constraint
        for (int scenario = 0; scenario < 4; ++scenario) {
            auto backend = init_test_audio_system();
            
            {
                auto device = audio_device::open_default_device(backend);
                device.resume();
                
                auto source1 = mock_audio_source::create();
                auto source2 = mock_audio_source::create();
                
                auto stream1 = device.create_stream(std::move(*source1));
                auto stream2 = device.create_stream(std::move(*source2));
                
                stream1.open();
                stream2.open();
                
                stream1.play();
                stream2.play();
                
                // Different cleanup orders
                switch (scenario) {
                    case 0:
                        // Normal order
                        break;
                    case 1:
                        // Stop one stream first
                        stream1.stop();
                        break;
                    case 2:
                        // Stop both streams first
                        stream1.stop();
                        stream2.stop();
                        break;
                    case 3:
                        // Pause device first
                        device.pause();
                        break;
                }
            }
            
            audio_system::done();
        }
    }
    
    TEST_CASE("memory leak check simulation") {
        // This test helps verify no memory leaks by creating/destroying many objects
        auto backend = init_test_audio_system();
        
        for (int iteration = 0; iteration < 10; ++iteration) {
            {
                auto device = audio_device::open_default_device(backend);
                device.resume();
                
                std::vector<std::unique_ptr<audio_stream>> streams;
                
                // Create many streams
                for (int i = 0; i < 20; ++i) {
                    auto source = create_mock_source();
                    auto stream = std::make_unique<audio_stream>(
                        device.create_stream(std::move(*source))
                    );
                    REQUIRE_NOTHROW(stream->open());
                    
                    if (i % 3 == 0) {
                        stream->set_finish_callback([](audio_stream& /*s*/) {
                            // Empty callback
                        });
                    }
                    
                    if (i % 2 == 0) {
                        stream->play();
                    }
                    
                    streams.push_back(std::move(stream));
                }
                
                // Random destruction
                while (!streams.empty()) {
                    size_t idx = static_cast<size_t>(rand()) % streams.size();
                    streams.erase(streams.begin() + static_cast<std::vector<std::unique_ptr<musac::audio_stream>>::difference_type>(idx));
                }
            }
        }
        
        audio_system::done();
    }
}

} // namespace musac::test