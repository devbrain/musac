// Audio callback performance benchmark
#include <nanobench.h>
#include "../benchmark_helpers.hh"
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/test_data/loader.hh>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>

namespace musac::benchmark {

// Benchmark the audio callback performance by measuring operations
// during actual audio playback
void register_audio_callback_benchmarks(ankerl::nanobench::Bench& bench) {
    // Initialize test data loader
    test_data::loader::init();
    
    // Test data types
    const std::vector<test_data::music_type> test_types = {
        test_data::music_type::mp3,
        test_data::music_type::voc,
        test_data::music_type::opb
    };
    
    // Create a single device for ALL benchmarks
    benchmark_device_guard device_guard;
    auto& device = device_guard.get();
    
    // === Measure callback performance during actual playback ===
    
    // Helper to measure operations while streams are playing
    auto measure_during_playback = [&](const std::string& name, size_t stream_count, 
                                       const std::function<void(std::vector<audio_stream>&)>& operation) {
        
        // Setup: Create and start streams
        std::vector<audio_stream> streams;
        std::atomic<int> callbacks_completed{0};
        
        for (size_t i = 0; i < stream_count; ++i) {
            auto src = test_data::loader::load(test_types[i % test_types.size()]);
            auto stream = device.create_stream(std::move(src));
            stream.open();
            
            // Set loop callback to count iterations
            stream.set_loop_callback([&callbacks_completed](audio_stream&) {
                callbacks_completed.fetch_add(1);
            });
            
            streams.push_back(std::move(stream));
        }
        
        // Start all streams playing (infinite loop)
        for (auto& stream : streams) {
            stream.play(0);  // 0 = infinite loop
        }
        
        // Let streams play for a bit to stabilize
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Measure the operation while streams are actively playing
        bench.run(name, [&] {
            operation(streams);
        });
        
        // Stop all streams using callbacks for clean shutdown
        std::vector<std::promise<void>> stop_promises;
        std::vector<std::future<void>> stop_futures;
        
        for (size_t i = 0; i < streams.size(); ++i) {
            stop_promises.emplace_back();
            stop_futures.push_back(stop_promises.back().get_future());
            
            streams[i].set_finish_callback([&stop_promises, i](audio_stream&) {
                stop_promises[i].set_value();
            });
            
            streams[i].stop();
        }
        
        // Wait for all streams to finish
        for (auto& future : stop_futures) {
            future.wait_for(std::chrono::milliseconds(500));
        }
    };
    
    // === Parameter Update Performance During Playback ===
    
    // Test volume updates with different numbers of active streams
    for (size_t stream_count : {1, 5, 10, 20}) {
        measure_during_playback(
            "AudioCallback/VolumeUpdate/" + std::to_string(stream_count) + "Streams",
            stream_count,
            [](std::vector<audio_stream>& streams) {
                // Update volume on first stream while others are playing
                if (!streams.empty()) {
                    streams[0].set_volume(0.5f + static_cast<float>(rand() % 50) / 100.0f);
                }
            }
        );
    }
    
    // Test stereo position updates
    for (size_t stream_count : {1, 5, 10, 20}) {
        measure_during_playback(
            "AudioCallback/StereoUpdate/" + std::to_string(stream_count) + "Streams",
            stream_count,
            [](std::vector<audio_stream>& streams) {
                if (!streams.empty()) {
                    streams[0].set_stereo_position(static_cast<float>(rand() % 200 - 100) / 100.0f);
                }
            }
        );
    }
    
    // === Query Performance During Playback ===
    
    // Test is_playing checks with active streams
    for (size_t stream_count : {1, 5, 10, 20}) {
        measure_during_playback(
            "AudioCallback/IsPlayingCheck/" + std::to_string(stream_count) + "Streams",
            stream_count,
            [](std::vector<audio_stream>& streams) {
                for (auto& stream : streams) {
                    ankerl::nanobench::doNotOptimizeAway(stream.is_playing());
                }
            }
        );
    }
    
    // Test volume queries
    for (size_t stream_count : {1, 5, 10, 20}) {
        measure_during_playback(
            "AudioCallback/VolumeQuery/" + std::to_string(stream_count) + "Streams",
            stream_count,
            [](std::vector<audio_stream>& streams) {
                for (auto& stream : streams) {
                    ankerl::nanobench::doNotOptimizeAway(stream.volume());
                }
            }
        );
    }
    
    // === Stream Addition/Removal During Playback ===
    
    bench.run("AudioCallback/StreamAddWhilePlaying", [&] {
        // Setup: Start some background streams
        std::vector<audio_stream> bg_streams;
        for (int i = 0; i < 5; ++i) {
            auto src = test_data::loader::load(test_types[i % test_types.size()]);
            auto stream = device.create_stream(std::move(src));
            stream.open();
            stream.play(0);
            bg_streams.push_back(std::move(stream));
        }
        
        // Measure: Add a new stream while others are playing
        auto src = test_data::loader::load(test_types[0]);
        auto new_stream = device.create_stream(std::move(src));
        new_stream.open();
        new_stream.play(1);  // Play once
        
        // Wait for completion
        std::promise<void> done;
        auto future = done.get_future();
        new_stream.set_finish_callback([&done](audio_stream&) {
            done.set_value();
        });
        
        future.wait_for(std::chrono::milliseconds(500));
        
        // Cleanup background streams
        for (auto& stream : bg_streams) {
            stream.stop();
        }
    });
    
    // === Mixed Operations During Playback ===
    
    // Simulate a typical game scenario with mixed operations
    measure_during_playback(
        "AudioCallback/GameScenario/8Streams",
        8,
        [](std::vector<audio_stream>& streams) {
            // Mix of operations that might happen in a game
            static int counter = 0;
            counter++;
            
            // Check playing state on all streams
            for (auto& stream : streams) {
                ankerl::nanobench::doNotOptimizeAway(stream.is_playing());
            }
            
            // Update volume on some streams
            if (counter % 10 == 0 && streams.size() >= 3) {
                streams[0].set_volume(0.8f);
                streams[1].set_volume(0.6f);
                streams[2].set_volume(0.4f);
            }
            
            // Update stereo position occasionally
            if (counter % 20 == 0 && !streams.empty()) {
                streams[0].set_stereo_position(-0.5f);
            }
            
            // Mute/unmute occasionally
            if (counter % 50 == 0 && streams.size() >= 2) {
                streams[1].mute();
                streams[1].unmute();
            }
        }
    );
    
    // === Stress Test: Many Streams ===
    
    // Test with a large number of concurrent streams
    // This test measures the overhead of managing many streams simultaneously
    bench.run("AudioCallback/StressTest/50Streams", [&] {
        // Use atomic counter instead of promises to avoid race conditions
        std::atomic<int> completed_count{0};
        std::vector<audio_stream> streams;
        
        // Create and start 50 streams
        for (int i = 0; i < 50; ++i) {
            auto src = test_data::loader::load(test_types[i % test_types.size()]);
            auto stream = device.create_stream(std::move(src));
            stream.open();
            
            // Use atomic counter for completion tracking
            stream.set_finish_callback([&completed_count](audio_stream&) {
                completed_count.fetch_add(1);
            });
            
            stream.play(1);  // Play once
            streams.push_back(std::move(stream));
        }
        
        // Wait for all streams to complete or timeout
        auto start_time = std::chrono::steady_clock::now();
        while (completed_count.load() < 50) {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > std::chrono::seconds(10)) {
                break;  // Timeout after 10 seconds
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Ensure all streams are stopped
        for (auto& stream : streams) {
            if (stream.is_playing()) {
                stream.stop();
            }
        }
        
        // Brief pause to let audio threads finish
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    });
    
    // === Baseline: Operations Without Playback ===
    
    // Measure operations without active playback for comparison
    bench.run("AudioCallback/Baseline/VolumeUpdate", [&] {
        auto src = test_data::loader::load(test_types[0]);
        auto stream = device.create_stream(std::move(src));
        stream.open();
        stream.set_volume(0.75f);
    });
    
    bench.run("AudioCallback/Baseline/IsPlayingCheck", [&] {
        auto src = test_data::loader::load(test_types[0]);
        auto stream = device.create_stream(std::move(src));
        stream.open();
        ankerl::nanobench::doNotOptimizeAway(stream.is_playing());
    });
    
    // Clean up test data loader
    test_data::loader::done();
}

} // namespace musac::benchmark