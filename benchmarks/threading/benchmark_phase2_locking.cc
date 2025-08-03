#include <nanobench.h>
#include "../benchmark_helpers.hh"
#include <vector>
#include <thread>
#include <atomic>

namespace musac::benchmark {

void register_phase2_locking_benchmarks(ankerl::nanobench::Bench& bench) {
    bench.title("Phase 2 - Fine-Grained Locking Performance");
    
    // Setup
    auto device = setup_benchmark_device();
    
    // Benchmark: Concurrent read operations (should scale well with shared_mutex)
    bench.run("concurrent_reads_1_thread", [&] {
        auto source = create_benchmark_source(44100 * 10);
        auto stream = device.create_stream(std::move(source));
        stream.open();
        stream.play();
        
        for (int i = 0; i < 1000; ++i) {
            auto vol = stream.volume();
            ankerl::nanobench::doNotOptimizeAway(vol);
        }
    });
    
    bench.run("concurrent_reads_4_threads", [&] {
        auto source = create_benchmark_source(44100 * 10);
        auto stream = std::make_shared<audio_stream>(device.create_stream(std::move(source)));
        stream->open();
        stream->play();
        
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([stream] {
                for (int i = 0; i < 250; ++i) {
                    auto vol = stream->volume();
                    ankerl::nanobench::doNotOptimizeAway(vol);
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    });
    
    // Benchmark: Mixed read/write operations
    bench.run("mixed_operations_1_thread", [&] {
        auto source = create_benchmark_source(44100 * 10);
        auto stream = device.create_stream(std::move(source));
        stream.open();
        stream.play();
        
        for (int i = 0; i < 100; ++i) {
            stream.set_volume(0.5f);
            auto vol = stream.volume();
            stream.set_stereo_position(0.0f);
            auto pos = stream.get_stereo_position();
            ankerl::nanobench::doNotOptimizeAway(vol);
            ankerl::nanobench::doNotOptimizeAway(pos);
        }
    });
    
    bench.run("mixed_operations_4_threads", [&] {
        auto source = create_benchmark_source(44100 * 10);
        auto stream = std::make_shared<audio_stream>(device.create_stream(std::move(source)));
        stream->open();
        stream->play();
        
        std::atomic<bool> stop{false};
        std::vector<std::thread> threads;
        
        // 2 reader threads
        for (int t = 0; t < 2; ++t) {
            threads.emplace_back([stream, &stop] {
                while (!stop) {
                    auto vol = stream->volume();
                    auto pos = stream->get_stereo_position();
                    ankerl::nanobench::doNotOptimizeAway(vol);
                    ankerl::nanobench::doNotOptimizeAway(pos);
                }
            });
        }
        
        // 2 writer threads
        for (int t = 0; t < 2; ++t) {
            threads.emplace_back([stream, &stop] {
                float vol = 0.1f;
                while (!stop) {
                    stream->set_volume(vol);
                    stream->set_stereo_position(vol - 0.5f);
                    vol += 0.1f;
                    if (vol > 1.0f) vol = 0.1f;
                }
            });
        }
        
        // Let threads run for a fixed time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        stop = true;
        
        for (auto& t : threads) {
            t.join();
        }
    });
    
    // Benchmark: State change operations (use exclusive state_lock)
    bench.run("state_changes_sequential", [&] {
        auto source = create_benchmark_source(44100 * 10);
        auto stream = device.create_stream(std::move(source));
        stream.open();
        
        for (int i = 0; i < 10; ++i) {
            stream.play();
            stream.pause();
            stream.resume();
            stream.stop();
        }
    });
    
    // Benchmark: Lock overhead comparison
    // This compares the overhead of different lock types
    const int ITERATIONS = 10000;
    
    bench.run("no_lock_baseline", [&] {
        float value = 0.5f;
        for (int i = 0; i < ITERATIONS; ++i) {
            ankerl::nanobench::doNotOptimizeAway(value);
        }
    });
    
    auto source = create_benchmark_source(44100 * 10);
    auto stream = device.create_stream(std::move(source));
    stream.open();
    
    bench.run("read_lock_overhead", [&] {
        for (int i = 0; i < ITERATIONS; ++i) {
            auto vol = stream.volume();
            ankerl::nanobench::doNotOptimizeAway(vol);
        }
    });
    
    bench.run("write_lock_overhead", [&] {
        for (int i = 0; i < ITERATIONS; ++i) {
            stream.set_volume(0.5f);
        }
    });
}

} // namespace musac::benchmark