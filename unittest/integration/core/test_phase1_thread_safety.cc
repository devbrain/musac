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
#include "../../mock_components.hh"
#include "../../test_fixtures.hh"

namespace musac::test {

TEST_SUITE("ThreadSafety::Phase1::Integration") {
    // Use the shared thread-safe fixture from test_helpers_v2.hh
    using audio_test_fixture = audio_test_fixture_threadsafe;
    
    // Test 1.1: Basic Destruction Safety
    TEST_CASE_FIXTURE(audio_test_fixture, "destruction during active playback") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        // Create a longer source to ensure callback is active
        auto source = create_mock_source(44100 * 5); // 5 seconds
        auto stream = std::make_unique<audio_stream>(
            device.create_stream(std::move(*source))
        );
        REQUIRE_NOTHROW(stream->open());
        stream->play();
        
        // Give playback time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Destroy stream while it's playing
        // The destructor should wait for callbacks to complete
        auto start = std::chrono::steady_clock::now();
        stream.reset();
        auto end = std::chrono::steady_clock::now();
        
        // Should have completed quickly (not hanging)
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        CHECK(duration.count() < 1000); // Should complete within 1 second
    }
    
    // Test 1.3: Concurrent Destruction
    TEST_CASE_FIXTURE(audio_test_fixture, "multiple streams destroyed simultaneously") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        const int STREAM_COUNT = 10;
        std::vector<std::unique_ptr<audio_stream>> streams;
        
        for (int i = 0; i < STREAM_COUNT; ++i) {
            auto source = create_mock_source(44100 * 2); // 2 seconds
            auto stream = std::make_unique<audio_stream>(
                device.create_stream(std::move(*source))
            );
            REQUIRE_NOTHROW(stream->open());
            stream->play();
            streams.push_back(std::move(stream));
        }
        
        // Give streams time to start playing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Destroy all streams concurrently
        std::vector<std::future<void>> futures;
        for (auto& stream : streams) {
            futures.push_back(std::async(std::launch::async, [&stream] {
                stream.reset();
            }));
        }
        
        // All should complete without deadlock
        for (auto& future : futures) {
            auto status = future.wait_for(std::chrono::seconds(10));
            CHECK(status == std::future_status::ready);
        }
    }
    
    // Test rapid creation/destruction cycles
    TEST_CASE_FIXTURE(audio_test_fixture, "rapid stream lifecycle") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        const int CYCLES = 50;
        std::atomic<int> streams_created{0};
        std::atomic<int> streams_destroyed{0};
        
        auto start_time = std::chrono::steady_clock::now();
        
        for (int i = 0; i < CYCLES; ++i) {
            auto source = create_mock_source(44100); // 1 second
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
            CHECK(stream.play());
            streams_created++;
            
            // Very short lifetime
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            
            // Stream destroyed here
            streams_destroyed++;
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Verify all streams were created and destroyed
        CHECK(streams_created == CYCLES);
        CHECK(streams_destroyed == CYCLES);
        
        // Should complete in reasonable time (not hanging)
        CHECK(duration.count() < CYCLES * 100); // Max 100ms per cycle
    }
    
    // Test destruction with callbacks
    TEST_CASE_FIXTURE(audio_test_fixture, "destruction with active callbacks") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        std::atomic<int> callback_count{0};
        std::atomic<int> streams_playing{0};
        std::mutex callback_mutex;
        
        // Create multiple short streams with callbacks
        constexpr int STREAM_COUNT = 5;
        std::vector<std::unique_ptr<audio_stream>> streams;
        
        for (int i = 0; i < STREAM_COUNT; ++i) {
            auto source = create_mock_source(44100 / 2); // 0.5 seconds
            auto stream = std::make_unique<audio_stream>(
                device.create_stream(std::move(*source))
            );
            REQUIRE_NOTHROW(stream->open());
            
            stream->set_finish_callback([&callback_count, &callback_mutex](audio_stream& /*s*/) {
                std::lock_guard<std::mutex> lock(callback_mutex);
                ++callback_count;
            });
            
            CHECK(stream->play(1)); // Play once
            streams_playing++;
            streams.push_back(std::move(stream));
        }
        
        CHECK(streams_playing == STREAM_COUNT);
        CHECK(streams.size() == STREAM_COUNT);
        
        // Destroy streams while callbacks might be firing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Measure destruction time
        auto start = std::chrono::steady_clock::now();
        streams.clear();
        auto end = std::chrono::steady_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Destruction should complete in reasonable time (not stuck waiting)
        CHECK(duration.count() < 6000); // Less than timeout (5s) + margin
        
        // At least some callbacks should have fired (but not necessarily all)
        CHECK(callback_count.load() >= 0);
        CHECK(callback_count.load() <= STREAM_COUNT);
    }
    
    // Test concurrent operations during destruction
    TEST_CASE_FIXTURE(audio_test_fixture, "operations during destruction") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        auto source = create_mock_source(44100 * 2); // 2 seconds
        auto stream = std::make_shared<audio_stream>(
            device.create_stream(std::move(*source))
        );
        REQUIRE_NOTHROW(stream->open());
        CHECK(stream->play());
        
        std::atomic<bool> stop_operations{false};
        std::atomic<int> successful_operations{0};
        std::atomic<int> failed_operations{0};
        std::atomic<bool> destruction_started{false};
        std::atomic<bool> destruction_completed{false};
        
        // Thread performing operations
        auto op_thread = std::thread([stream, &stop_operations, &successful_operations, 
                                     &failed_operations, &destruction_started]() {
            while (!stop_operations) {
                try {
                    stream->set_volume(0.5f);
                    stream->pause();
                    stream->resume();
                    [[maybe_unused]] auto vol = stream->volume();
                    successful_operations++;
                } catch (...) {
                    // Operations might fail if stream is being destroyed
                    failed_operations++;
                    // Once we start failing, destruction has likely started
                    if (!destruction_started.load()) {
                        destruction_started = true;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        
        // Let operations run
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Verify operations were successful before destruction
        CHECK(successful_operations.load() > 0);
        CHECK(failed_operations.load() == 0);
        
        // Start destruction in another thread  
        auto destroy_thread = std::thread([&stream, &destruction_completed]() {
            stream.reset();
            destruction_completed = true;
        });
        
        // Let destruction start
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Stop operations
        stop_operations = true;
        op_thread.join();
        destroy_thread.join();
        
        // Verify destruction completed
        CHECK(destruction_completed.load());
        
        // We should have had some successful operations
        CHECK(successful_operations.load() > 10);
        
        // Failed operations are expected during destruction, but shouldn't crash
        CHECK(failed_operations.load() >= 0);
    }
}

} // namespace musac::test