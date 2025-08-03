#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <thread>
#include <vector>
#include <atomic>
#include <future>
#include <chrono>
#include <memory>
#include "../test_helpers.hh"

namespace musac::test {

TEST_SUITE("phase4_sdl_callback_safety") {
    struct audio_test_fixture {
        audio_test_fixture() {
            audio_system::init();
        }
        
        ~audio_test_fixture() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            audio_system::done();
        }
    };
    
    // Test 4.1: Basic SDL Stream Destruction During Callback
    TEST_CASE_FIXTURE(audio_test_fixture, "sdl stream destruction during active callback") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        // Create a source that generates continuous audio
        auto source = create_mock_source(44100 * 10); // 10 seconds
        auto stream = std::make_unique<audio_stream>(
            device.create_stream(std::move(*source))
        );
        
        CHECK(stream->open());
        CHECK(stream->play());
        
        // Let the stream play for a bit to ensure callbacks are active
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Destroy stream while callbacks are likely active
        auto start = std::chrono::steady_clock::now();
        stream.reset();
        auto end = std::chrono::steady_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Should complete quickly without hanging
        CHECK(duration.count() < 1000);
        
        // Give time to verify no crashes after destruction
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Test 4.2: Rapid Stream Creation/Destruction
    TEST_CASE_FIXTURE(audio_test_fixture, "rapid sdl stream lifecycle") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        const int CYCLES = 20;
        
        for (int i = 0; i < CYCLES; ++i) {
            auto source = create_mock_source(44100 * 2); // 2 seconds
            auto stream = device.create_stream(std::move(*source));
            
            CHECK(stream.open());
            CHECK(stream.play());
            
            // Very short playback time
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Stream destroyed here - SDL callbacks should be synchronized
        }
        
        // No crashes means synchronization is working
        CHECK(true);
    }
    
    // Test 4.3: Concurrent Stream Operations with SDL Backend
    TEST_CASE_FIXTURE(audio_test_fixture, "concurrent sdl stream operations") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        const int STREAM_COUNT = 10;
        std::vector<std::unique_ptr<audio_stream>> streams;
        
        // Create multiple streams
        for (int i = 0; i < STREAM_COUNT; ++i) {
            auto source = create_mock_source(44100 * 5); // 5 seconds
            auto stream = std::make_unique<audio_stream>(
                device.create_stream(std::move(*source))
            );
            stream->open();
            stream->play();
            streams.push_back(std::move(stream));
        }
        
        // Let them play
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Destroy streams concurrently
        std::vector<std::future<void>> futures;
        for (auto& stream : streams) {
            futures.push_back(std::async(std::launch::async, [&stream] {
                stream.reset();
            }));
        }
        
        // All should complete without deadlock
        for (auto& future : futures) {
            auto status = future.wait_for(std::chrono::seconds(5));
            CHECK(status == std::future_status::ready);
        }
    }
    
    // Test 4.4: Stream Destruction During Heavy Callback Load
    TEST_CASE_FIXTURE(audio_test_fixture, "sdl destruction under heavy load") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        // Create multiple streams to increase callback frequency
        const int STREAM_COUNT = 20;
        std::vector<std::unique_ptr<audio_stream>> streams;
        
        for (int i = 0; i < STREAM_COUNT; ++i) {
            // Use small buffer sources to trigger more callbacks
            auto source = create_mock_source(512); // Very small buffer
            auto stream = std::make_unique<audio_stream>(
                device.create_stream(std::move(*source))
            );
            stream->open();
            stream->play(0); // Loop forever
            streams.push_back(std::move(stream));
        }
        
        // Let callbacks run heavily
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Now destroy all streams - should handle callback synchronization
        auto start = std::chrono::steady_clock::now();
        streams.clear();
        auto end = std::chrono::steady_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Should complete in reasonable time even under heavy load
        CHECK(duration.count() < 2000);
    }
    
    // Test 4.5: Pause/Resume During Destruction
    TEST_CASE_FIXTURE(audio_test_fixture, "sdl pause resume during destruction") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        auto source = create_mock_source(44100 * 5); // 5 seconds
        auto stream = std::make_shared<audio_stream>(
            device.create_stream(std::move(*source))
        );
        stream->open();
        stream->play();
        
        std::atomic<bool> stop_operations{false};
        
        // Thread doing pause/resume
        auto op_thread = std::thread([stream, &stop_operations]() {
            while (!stop_operations) {
                stream->pause();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                stream->resume();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        
        // Let operations run
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Start destruction
        auto destroy_thread = std::thread([&stream]() {
            stream.reset();
        });
        
        // Stop operations after a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        stop_operations = true;
        
        op_thread.join();
        destroy_thread.join();
        
        // Should complete without crashes
        CHECK(true);
    }
}

} // namespace musac::test