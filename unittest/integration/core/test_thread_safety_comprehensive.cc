/**
 * Comprehensive Thread Safety Tests
 * 
 * This file consolidates thread safety tests from:
 * - test_thread_safety.cc
 * - test_phase1_thread_safety.cc
 * - test_internal_mixer_safety.cc
 * - test_phase4_sdl_callback_safety.cc
 * - test_phase5_integration.cc
 * 
 * Tests are organized by category for better maintainability.
 */

#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <mutex>
#include <memory>
#include <future>
#include <chrono>
#include <set>
#include "../../mock_components.hh"
#include "../../test_fixtures.hh"

namespace musac::test {

TEST_SUITE("ThreadSafety::Comprehensive::Integration") {
    using audio_test_fixture = audio_test_fixture_threadsafe;
    
    //========================================================================
    // Stream Creation and Destruction Tests
    //========================================================================
    
    TEST_CASE_FIXTURE(audio_test_fixture, "concurrent stream creation") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        run_concurrent_test([&device]() {
            auto source = create_mock_source(44100);
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
        }, 10, 1);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "concurrent stream destruction") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        std::vector<std::unique_ptr<audio_stream>> streams;
        std::mutex streams_mutex;
        
        // Create streams
        for (int i = 0; i < 10; ++i) {
            auto source = create_mock_source(44100);
            auto stream = device.create_stream(std::move(*source));
            stream.open();
            stream.play();
            streams.push_back(std::make_unique<audio_stream>(std::move(stream)));
        }
        
        // Destroy streams concurrently
        run_concurrent_test([&streams, &streams_mutex]() {
            std::unique_ptr<audio_stream> stream;
            {
                std::lock_guard<std::mutex> lock(streams_mutex);
                if (!streams.empty()) {
                    stream = std::move(streams.back());
                    streams.pop_back();
                }
            }
            // Stream destroyed here
        }, 5, 2);
        
        CHECK(streams.empty());
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "rapid stream create/destroy cycles") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        run_concurrent_test([&device]() {
            for (int i = 0; i < 10; ++i) {
                auto source = create_mock_source(1024);
                auto stream = device.create_stream(std::move(*source));
                stream.open();
                stream.play();
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                stream.stop();
            }
        }, 4, 1);
    }
    
    //========================================================================
    // Mixer Thread Safety Tests
    //========================================================================
    
    TEST_CASE_FIXTURE(audio_test_fixture, "mixer concurrent stream operations") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        std::vector<std::shared_ptr<audio_stream>> streams;
        std::mutex streams_mutex;
        std::atomic<int> operations_count{0};
        
        // Thread 1-3: Create streams
        std::vector<std::thread> creators;
        for (int t = 0; t < 3; ++t) {
            creators.emplace_back([&]() {
                for (int i = 0; i < 5; ++i) {
                    auto source = create_mock_source(44100);
                    auto stream = std::make_shared<audio_stream>(
                        device.create_stream(std::move(*source)));
                    stream->open();
                    stream->play();
                    
                    std::lock_guard<std::mutex> lock(streams_mutex);
                    streams.push_back(stream);
                    operations_count++;
                }
            });
        }
        
        // Thread 4-6: Control streams
        std::vector<std::thread> controllers;
        for (int t = 0; t < 3; ++t) {
            controllers.emplace_back([&]() {
                for (int i = 0; i < 20; ++i) {
                    std::shared_ptr<audio_stream> stream;
                    {
                        std::lock_guard<std::mutex> lock(streams_mutex);
                        if (!streams.empty()) {
                            stream = streams[i % streams.size()];
                        }
                    }
                    
                    if (stream) {
                        if (i % 3 == 0) stream->pause();
                        else if (i % 3 == 1) stream->resume();
                        else stream->set_volume(0.5f);
                        operations_count++;
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            });
        }
        
        // Wait for creators first
        for (auto& t : creators) {
            t.join();
        }
        
        // Then controllers
        for (auto& t : controllers) {
            t.join();
        }
        
        CHECK(operations_count > 0);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "mixer stream addition during callback") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        std::atomic<bool> callback_running{false};
        std::atomic<int> callback_count{0};
        
        // Create initial stream with callback that tracks execution
        device.create_stream_with_callback(
            [](void* userdata, uint8_t*, int) {
                auto* data = static_cast<std::pair<std::atomic<bool>*, std::atomic<int>*>*>(userdata);
                data->first->store(true);
                data->second->fetch_add(1);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            },
            new std::pair<std::atomic<bool>*, std::atomic<int>*>(&callback_running, &callback_count)
        );
        
        // Wait for callback to start
        while (!callback_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Add more streams while callback is running
        run_concurrent_test([&device]() {
            auto source = create_mock_source(1024);
            auto stream = device.create_stream(std::move(*source));
            stream.open();
            stream.play();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }, 3, 2);
        
        CHECK(callback_count > 0);
    }
    
    //========================================================================
    // Device Operations Thread Safety
    //========================================================================
    
    TEST_CASE_FIXTURE(audio_test_fixture, "concurrent device control operations") {
        auto device = audio_device::open_default_device(backend);
        
        std::atomic<int> pause_count{0};
        std::atomic<int> resume_count{0};
        std::atomic<int> gain_count{0};
        
        std::vector<std::thread> threads;
        
        // Pause/resume threads
        for (int i = 0; i < 2; ++i) {
            threads.emplace_back([&device, &pause_count, &resume_count]() {
                for (int j = 0; j < 10; ++j) {
                    device.pause();
                    pause_count++;
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    device.resume();
                    resume_count++;
                }
            });
        }
        
        // Gain adjustment threads
        for (int i = 0; i < 2; ++i) {
            threads.emplace_back([&device, &gain_count]() {
                for (int j = 0; j < 20; ++j) {
                    device.set_gain(0.5f + (j % 10) * 0.05f);
                    gain_count++;
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        CHECK(pause_count == 20);
        CHECK(resume_count == 20);
        CHECK(gain_count == 40);
    }
    
    //========================================================================
    // Complex Integration Tests
    //========================================================================
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stress test - concurrent everything") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        std::atomic<bool> stop_flag{false};
        std::atomic<int> total_operations{0};
        std::vector<std::shared_ptr<audio_stream>> active_streams;
        std::mutex streams_mutex;
        
        // Stream creator thread
        std::thread creator([&]() {
            while (!stop_flag) {
                auto source = create_mock_source(4096);
                auto stream = std::make_shared<audio_stream>(
                    device.create_stream(std::move(*source)));
                stream->open();
                stream->play();
                
                {
                    std::lock_guard<std::mutex> lock(streams_mutex);
                    active_streams.push_back(stream);
                }
                total_operations++;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
        
        // Stream destroyer thread
        std::thread destroyer([&]() {
            while (!stop_flag) {
                std::shared_ptr<audio_stream> stream;
                {
                    std::lock_guard<std::mutex> lock(streams_mutex);
                    if (!active_streams.empty()) {
                        stream = active_streams.back();
                        active_streams.pop_back();
                    }
                }
                if (stream) {
                    stream->stop();
                    total_operations++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(7));
            }
        });
        
        // Stream controller thread
        std::thread controller([&]() {
            while (!stop_flag) {
                std::vector<std::shared_ptr<audio_stream>> streams_copy;
                {
                    std::lock_guard<std::mutex> lock(streams_mutex);
                    streams_copy = active_streams;
                }
                
                for (auto& stream : streams_copy) {
                    if (stream) {
                        stream->set_volume(0.5f);
                        total_operations++;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }
        });
        
        // Device controller thread
        std::thread device_controller([&]() {
            while (!stop_flag) {
                device.pause();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                device.resume();
                total_operations++;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Run for 200ms
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        stop_flag = true;
        
        creator.join();
        destroyer.join();
        controller.join();
        device_controller.join();
        
        CHECK(total_operations > 0);
        
        // Cleanup remaining streams
        active_streams.clear();
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "deadlock prevention - circular operations") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        // This test attempts operations that could deadlock if not properly handled
        std::vector<std::shared_ptr<audio_stream>> streams;
        
        // Create initial streams
        for (int i = 0; i < 5; ++i) {
            auto source = create_mock_source(2048);
            auto stream = std::make_shared<audio_stream>(
                device.create_stream(std::move(*source)));
            stream->open();
            stream->play();
            streams.push_back(stream);
        }
        
        std::atomic<bool> deadlock_detected{false};
        
        // Thread 1: Pause device while iterating streams
        std::thread t1([&]() {
            auto future = std::async(std::launch::async, [&]() {
                device.pause();
                for (auto& stream : streams) {
                    stream->pause();
                }
                device.resume();
            });
            
            if (future.wait_for(std::chrono::seconds(1)) == std::future_status::timeout) {
                deadlock_detected = true;
            }
        });
        
        // Thread 2: Modify streams while device operations
        std::thread t2([&]() {
            auto future = std::async(std::launch::async, [&]() {
                for (auto& stream : streams) {
                    stream->set_volume(0.5f);
                    device.set_gain(0.8f);
                }
            });
            
            if (future.wait_for(std::chrono::seconds(1)) == std::future_status::timeout) {
                deadlock_detected = true;
            }
        });
        
        t1.join();
        t2.join();
        
        CHECK(!deadlock_detected);
    }
}

} // namespace musac::test