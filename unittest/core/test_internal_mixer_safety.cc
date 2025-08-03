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
#include <random>
#include <set>
#include "../test_helpers.hh"

// Only include internal headers if we have access to them
#ifdef MUSAC_INTERNAL_TESTING
#include "../../src/musac/audio_mixer.hh"
#endif

namespace musac::test {

TEST_SUITE("internal_mixer_thread_safety") {
    struct audio_test_fixture {
        audio_test_fixture() {
            audio_system::init();
        }
        
        ~audio_test_fixture() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            audio_system::done();
        }
    };

#ifdef MUSAC_INTERNAL_TESTING
    // Direct mixer tests - only available with internal testing
    
    TEST_CASE_FIXTURE(audio_test_fixture, "direct mixer concurrent operations") {
        audio_mixer mixer;
        const int OPERATIONS = 1000;
        const int THREADS = 8;
        
        // Create a pool of mock streams
        std::vector<std::unique_ptr<audio_stream>> streams;
        auto device = audio_device::open_default_device();
        device.resume();
        
        for (int i = 0; i < OPERATIONS; ++i) {
            auto source = create_mock_source(44100);
            streams.push_back(std::make_unique<audio_stream>(
                device.create_stream(std::move(*source))
            ));
        }
        
        std::vector<std::future<void>> futures;
        
        // Add threads
        for (int t = 0; t < THREADS; ++t) {
            futures.push_back(std::async(std::launch::async, [&mixer, &streams, t, OPERATIONS, THREADS] {
                int start = (t * OPERATIONS) / THREADS;
                int end = ((t + 1) * OPERATIONS) / THREADS;
                
                for (int i = start; i < end; ++i) {
                    mixer.add_stream(streams[i].get());
                    std::this_thread::yield();
                }
            }));
        }
        
        // Wait for all operations
        for (auto& future : futures) {
            auto status = future.wait_for(std::chrono::seconds(10));
            CHECK(status == std::future_status::ready);
        }
        
        // Verify final state
        auto final_streams = mixer.get_streams();
        CHECK(final_streams->size() == OPERATIONS);
        
        // Check no duplicates
        std::set<audio_stream*> unique_streams(final_streams->begin(), final_streams->end());
        CHECK(unique_streams.size() == final_streams->size());
    }
    
#endif // MUSAC_INTERNAL_TESTING

    // Public API tests - always available
    
    TEST_CASE_FIXTURE(audio_test_fixture, "concurrent stream creation and playback") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        const int STREAM_COUNT = 50;
        const int THREAD_COUNT = 8;
        
        std::atomic<int> playing_count{0};
        std::vector<std::future<std::vector<audio_stream>>> futures;
        
        // Create streams concurrently
        for (int t = 0; t < THREAD_COUNT; ++t) {
            futures.push_back(std::async(std::launch::async, [&device, &playing_count, t, STREAM_COUNT, THREAD_COUNT] {
                std::vector<audio_stream> local_streams;
                int start = (t * STREAM_COUNT) / THREAD_COUNT;
                int end = ((t + 1) * STREAM_COUNT) / THREAD_COUNT;
                
                for (int i = start; i < end; ++i) {
                    auto source = create_mock_source(44100 * 2); // 2 seconds
                    auto stream = device.create_stream(std::move(*source));
                    REQUIRE_NOTHROW(stream.open());
                    stream.play();
                    playing_count++;
                    local_streams.push_back(std::move(stream));
                }
                
                return local_streams;
            }));
        }
        
        // Collect all streams
        std::vector<audio_stream> all_streams;
        for (auto& future : futures) {
            auto streams = future.get();
            for (auto& stream : streams) {
                all_streams.push_back(std::move(stream));
            }
        }
        
        CHECK(all_streams.size() == STREAM_COUNT);
        CHECK(playing_count.load() == STREAM_COUNT);
        
        // Verify all are playing
        for (const auto& stream : all_streams) {
            CHECK(stream.is_playing());
        }
        
        // Concurrent operations on playing streams
        std::vector<std::thread> workers;
        std::atomic<bool> stop{false};
        std::atomic<int> operations{0};
        
        for (int t = 0; t < 4; ++t) {
            workers.emplace_back([&all_streams, &stop, &operations] {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, all_streams.size() - 1);
                std::uniform_real_distribution<float> vol_dis(0.0f, 1.0f);
                
                while (!stop) {
                    int idx = dis(gen);
                    
                    // Random operations
                    switch (dis(gen) % 4) {
                        case 0:
                            all_streams[idx].set_volume(vol_dis(gen));
                            break;
                        case 1:
                            all_streams[idx].mute();
                            break;
                        case 2:
                            all_streams[idx].unmute();
                            break;
                        case 3:
                            all_streams[idx].pause();
                            all_streams[idx].resume();
                            break;
                    }
                    operations++;
                }
            });
        }
        
        // Let it run
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        stop = true;
        
        for (auto& w : workers) {
            w.join();
        }
        
        // Should have performed many operations without crashes
        CHECK(operations.load() > 1000);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream destruction during playback") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        // Create streams in threads and destroy them while playing
        std::vector<std::thread> threads;
        std::atomic<int> created{0};
        std::atomic<int> destroyed{0};
        std::atomic<bool> stop{false};
        
        // Creator threads
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&device, &created, &destroyed, &stop] {
                while (!stop) {
                    auto source = create_mock_source(44100);
                    auto stream = device.create_stream(std::move(*source));
                    REQUIRE_NOTHROW(stream.open());
                    stream.play();
                    created++;
                    
                    // Let it play briefly
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    
                    // Stream destroyed here
                    destroyed++;
                }
            });
        }
        
        // Run for a while
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        stop = true;
        
        for (auto& t : threads) {
            t.join();
        }
        
        CHECK(created.load() > 50);
        CHECK(destroyed.load() == created.load());
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "rapid pause resume cycles") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        // Create multiple streams
        std::vector<audio_stream> streams;
        for (int i = 0; i < 10; ++i) {
            auto source = create_mock_source(44100 * 10); // 10 seconds
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
            stream.play();
            streams.push_back(std::move(stream));
        }
        
        // Rapid pause/resume in multiple threads
        std::vector<std::thread> threads;
        std::atomic<bool> stop{false};
        std::atomic<int> cycles{0};
        
        for (size_t i = 0; i < streams.size(); ++i) {
            threads.emplace_back([&streams, i, &stop, &cycles] {
                while (!stop) {
                    streams[i].pause();
                    std::this_thread::yield();
                    streams[i].resume();
                    cycles++;
                }
            });
        }
        
        // Let it run
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        stop = true;
        
        for (auto& t : threads) {
            t.join();
        }
        
        CHECK(cycles.load() > 1000);
        
        // All should still be valid
        for (auto& stream : streams) {
            CHECK((stream.is_playing() || stream.is_paused()));
        }
    }
}

} // namespace musac::test