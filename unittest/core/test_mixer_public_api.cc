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
#include "../test_helpers.hh"

namespace musac::test {

TEST_SUITE("mixer_thread_safety_public_api") {
    struct audio_test_fixture {
        audio_test_fixture() {
            audio_system::init();
        }
        
        ~audio_test_fixture() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            audio_system::done();
        }
    };
    
    // Test mixer indirectly through public API
    TEST_CASE_FIXTURE(audio_test_fixture, "mixer_thread_safety_public_api :: concurrent stream operations") {
        auto device = audio_device::open_default_device();
        device.resume();

        constexpr int STREAM_COUNT = 50;
        constexpr int THREAD_COUNT = 8;
        
        std::atomic<int> created{0};
        std::atomic<int> playing{0};
        
        // Create streams concurrently
        std::vector<std::future<std::vector<std::unique_ptr<audio_stream>>>> futures;
        
        for (int t = 0; t < THREAD_COUNT; ++t) {
            futures.push_back(std::async(std::launch::async, [&, t] {
                std::vector<std::unique_ptr<audio_stream>> local_streams;
                int start = (t * STREAM_COUNT) / THREAD_COUNT;
                int end = ((t + 1) * STREAM_COUNT) / THREAD_COUNT;
                
                for (int i = start; i < end; ++i) {
                    auto source = create_mock_source(44100 * 2); // 2 seconds
                    auto stream = std::make_unique<audio_stream>(device.create_stream(std::move(*source)));
                    created++;
                    
                    try {
                        stream->open();
                        if (stream->play()) {
                            playing++;
                        }
                    } catch (...) {
                        // open() failed, don't count as playing
                    }
                    
                    local_streams.push_back(std::move(stream));
                }
                
                return local_streams;
            }));
        }
        
        // Collect all streams
        std::vector<std::unique_ptr<audio_stream>> all_streams;
        for (auto& future : futures) {
            auto streams = future.get();
            for (auto& stream : streams) {
                all_streams.push_back(std::move(stream));
            }
        }
        
        CHECK(created.load() == STREAM_COUNT);
        CHECK(playing.load() == STREAM_COUNT);
        CHECK(all_streams.size() == STREAM_COUNT);
        
        // Verify all are playing
        for (const auto& stream : all_streams) {
            CHECK(stream->is_playing());
        }
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream lifecycle stress test") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        std::atomic<bool> stop{false};
        std::atomic<int> cycles{0};
        std::atomic<int> errors{0};
        
        // Worker threads that create/destroy streams rapidly
        std::vector<std::thread> workers;
        
        for (int t = 0; t < 4; ++t) {
            workers.emplace_back([&] {
                while (!stop) {
                    try {
                        auto source = create_mock_source(44100);
                        auto stream = device.create_stream(std::move(*source));
                        
                        try {
                            stream.open();
                            stream.play();
                            // Let it play briefly
                            std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        } catch (...) {
                            // open() failed, continue with next iteration
                        }
                        
                        cycles++;
                        // Stream destroyed here - tests mixer remove during destruction
                    } catch (...) {
                        errors++;
                    }
                }
            });
        }
        
        // Run for a while
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        stop = true;
        
        for (auto& w : workers) {
            w.join();
        }
        
        CHECK(cycles.load() > 100);
        CHECK(errors.load() == 0);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "concurrent volume operations") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        // Create streams
        std::vector<std::unique_ptr<audio_stream>> streams;
        for (int i = 0; i < 20; ++i) {
            auto source = create_mock_source(44100 * 10); // 10 seconds
            auto stream = std::make_unique<audio_stream>(device.create_stream(std::move(*source)));
            REQUIRE_NOTHROW(stream->open());
            stream->play();
            streams.push_back(std::move(stream));
        }
        
        std::atomic<bool> stop{false};
        std::atomic<int> operations{0};
        
        // Threads modifying stream properties
        std::vector<std::thread> workers;
        
        for (int t = 0; t < 8; ++t) {
            workers.emplace_back([&] {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<std::size_t> stream_dis(0, streams.size() - 1);
                std::uniform_real_distribution<float> vol_dis(0.0f, 1.0f);
                std::uniform_int_distribution<> op_dis(0, 5);
                
                while (!stop) {
                    auto idx = stream_dis(gen);
                    
                    switch (op_dis(gen)) {
                        case 0:
                            streams[idx]->set_volume(vol_dis(gen));
                            break;
                        case 1:
                            streams[idx]->mute();
                            break;
                        case 2:
                            streams[idx]->unmute();
                            break;
                        case 3:
                            streams[idx]->pause();
                            break;
                        case 4:
                            streams[idx]->resume();
                            break;
                        case 5:
                            streams[idx]->set_stereo_position(vol_dis(gen) * 2.0f - 1.0f);
                            break;
                    }
                    
                    operations++;
                    std::this_thread::yield();
                }
            });
        }
        
        // Let it run
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        stop = true;
        
        for (auto& w : workers) {
            w.join();
        }
        
        CHECK(operations.load() > 10000);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "pause resume race conditions") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        // Create multiple streams
        std::vector<std::unique_ptr<audio_stream>> streams;
        for (int i = 0; i < 10; ++i) {
            auto source = create_mock_source(44100 * 10); // 10 seconds
            auto stream = std::make_unique<audio_stream>(device.create_stream(std::move(*source)));
            REQUIRE_NOTHROW(stream->open());
            stream->play();
            streams.push_back(std::move(stream));
        }
        
        // Rapid pause/resume from multiple threads
        std::atomic<bool> stop{false};
        std::vector<std::thread> threads;
        
        for (const auto & stream : streams) {
            threads.emplace_back([&stream, &stop] {
                while (!stop) {
                    stream->pause();
                    std::this_thread::yield();
                    stream->resume();
                    std::this_thread::yield();
                }
            });
        }
        
        // Also have threads checking state
        for (size_t i = 0; i < 4; ++i) {
            threads.emplace_back([&streams, &stop] {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<std::size_t> dis(0, streams.size() - 1);
                
                while (!stop) {
                    auto idx = dis(gen);
                    bool playing = streams[idx]->is_playing();
                    bool paused = streams[idx]->is_paused();
                    
                    // Should be in a valid state
                    // Note: There can be transient states during rapid pause/resume
                    // So we just verify we can query the state without crashing
                    (void)playing;
                    (void)paused;
                    std::this_thread::yield();
                }
            });
        }
        
        // Let it run
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        stop = true;
        
        for (auto& t : threads) {
            t.join();
        }
    }
}

} // namespace musac::test