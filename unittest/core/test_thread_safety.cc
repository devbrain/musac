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
#include "../test_helpers.hh"
#include "../test_helpers_v2.hh"

namespace musac::test {

TEST_SUITE("thread_safety") {
    struct audio_test_fixture : test::audio_test_fixture_v2 {
        ~audio_test_fixture() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };
    
    TEST_CASE_FIXTURE(audio_test_fixture, "concurrent stream creation") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        std::vector<std::thread> threads;
        std::vector<std::unique_ptr<audio_stream>> streams;
        std::mutex streams_mutex;
        
        // Create streams from multiple threads
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&device, &streams, &streams_mutex]() {
                auto source = create_mock_source(44100);
                auto stream = device.create_stream(std::move(*source));
                REQUIRE_NOTHROW(stream.open());
                
                std::lock_guard<std::mutex> lock(streams_mutex);
                streams.push_back(std::make_unique<audio_stream>(std::move(stream)));
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        CHECK(streams.size() == 10);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "concurrent stream operations") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        auto source = create_mock_source(44100 * 10); // 10 seconds
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        
        std::atomic<bool> stop_flag{false};
        std::vector<std::thread> threads;
        
        // Thread 1: Play/Stop cycles
        threads.emplace_back([&stream, &stop_flag]() {
            while (!stop_flag) {
                stream.play();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                stream.stop();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Thread 2: Volume changes
        threads.emplace_back([&stream, &stop_flag]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(0.0f, 1.0f);
            
            while (!stop_flag) {
                stream.set_volume(dis(gen));
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
        
        // Thread 3: Pause/Resume cycles
        threads.emplace_back([&stream, &stop_flag]() {
            while (!stop_flag) {
                stream.pause();
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
                stream.resume();
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
            }
        });
        
        // Thread 4: Query state
        threads.emplace_back([&stream, &stop_flag]() {
            while (!stop_flag) {
                auto playing = stream.is_playing();
                auto paused = stream.is_paused();
                auto volume = stream.volume();
                (void)playing;
                (void)paused;
                (void)volume;
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }
        });
        
        // Let threads run for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        stop_flag = true;
        
        for (auto& t : threads) {
            t.join();
        }
        
        // Should not crash or deadlock
        CHECK(true);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "callback thread safety") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        std::atomic<int> callback_count{0};
        std::mutex callback_mutex;
        std::vector<int> callback_order;
        
        // Create multiple short streams
        std::vector<std::unique_ptr<audio_stream>> streams;
        for (int i = 0; i < 5; ++i) {
            auto source = create_mock_source(44100);
            auto stream = std::make_unique<audio_stream>(device.create_stream(std::move(*source)));
            REQUIRE_NOTHROW(stream->open());
            
            int stream_id = i;
            stream->set_finish_callback([&callback_count, &callback_mutex, &callback_order, stream_id](audio_stream& /*s*/) {
                callback_count++;
                
                std::lock_guard<std::mutex> lock(callback_mutex);
                callback_order.push_back(stream_id);
            });
            
            streams.push_back(std::move(stream));
        }
        
        // Play all streams
        for (auto& stream : streams) {
            stream->play(1); // Play once
        }
        
        // Concurrently modify callbacks while they might be executing
        std::thread modifier([&streams]() {
            for (int i = 0; i < 10; ++i) {
                for (auto& stream : streams) {
                    stream->set_volume(0.5f);
                    stream->set_stereo_position(0.0f);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        modifier.join();
        
        // No assertions on callback_count as timing is unpredictable
        // The test passes if it doesn't crash
        CHECK(true);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "device enumeration during playback") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        // Start playing a stream
        auto source = create_mock_source(44100);
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        stream.play();
        
        std::atomic<bool> stop_flag{false};
        std::vector<std::thread> threads;
        
        // Thread 1: Enumerate devices repeatedly
        threads.emplace_back([&stop_flag, this]() {
            while (!stop_flag) {
                auto devices = audio_device::enumerate_devices(backend, true);
                CHECK(devices.size() > 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Thread 2: Query device properties repeatedly
        threads.emplace_back([&stop_flag, &device]() {
            while (!stop_flag) {
                auto name = device.get_device_name();
                auto channels = device.get_channels();
                auto freq = device.get_freq();
                CHECK(!name.empty());
                CHECK(channels > 0);
                CHECK(freq > 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        });
        
        // Let it run
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        stop_flag = true;
        
        for (auto& t : threads) {
            t.join();
        }
        
        stream.stop();
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "rapid callback changes") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        auto source = create_mock_source(44100);
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        stream.play();
        
        std::atomic<int> callback_counter{0};
        
        // Rapidly change callbacks while playing
        for (int i = 0; i < 100; ++i) {
            if (i % 2 == 0) {
                stream.set_finish_callback([&callback_counter](audio_stream& /*s*/) {
                    callback_counter++;
                });
            } else {
                stream.remove_finish_callback();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        stream.stop();
        
        // Test passes if no crash occurs
        CHECK(true);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream destruction race") {
        auto device = audio_device::open_default_device(backend);
        device.resume();
        
        // Create and destroy streams rapidly from multiple threads
        std::atomic<bool> stop_flag{false};
        std::vector<std::thread> threads;
        
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&device, &stop_flag]() {
                while (!stop_flag) {
                    auto source = create_mock_source(44100);
                    auto stream = device.create_stream(std::move(*source));
                    REQUIRE_NOTHROW(stream.open());
                    stream.play();
                    
                    // Random short lifetime
                    std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 20));
                    
                    // Stream destroyed here
                }
            });
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        stop_flag = true;
        
        for (auto& t : threads) {
            t.join();
        }
        
        CHECK(true);
    }
}

} // namespace musac::test