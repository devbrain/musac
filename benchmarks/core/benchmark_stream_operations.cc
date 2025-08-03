#include <nanobench.h>
#include "../benchmark_helpers.hh"
#include <vector>
#include <thread>

namespace musac::benchmark {

void register_stream_operation_benchmarks(ankerl::nanobench::Bench& bench) {
    bench.title("Stream Operations");
    
    // Setup
    auto device = setup_benchmark_device();
    
    // Benchmark: Stream creation and destruction
    bench.run("stream_create_destroy", [&] {
        auto source = create_benchmark_source(44100);
        auto stream = device.create_stream(std::move(source));
        ankerl::nanobench::doNotOptimizeAway(stream);
    });
    
    // Benchmark: Stream open
    auto source1 = create_benchmark_source(44100 * 10);
    auto stream1 = device.create_stream(std::move(source1));
    bench.run("stream_open", [&] {
        stream1.open();
    });
    
    // Benchmark: Play/Stop cycles
    auto source2 = create_benchmark_source(44100 * 10);
    auto stream2 = device.create_stream(std::move(source2));
    stream2.open();
    bench.run("stream_play_stop", [&] {
        stream2.play();
        stream2.stop();
    });
    
    // Benchmark: Volume changes
    auto source3 = create_benchmark_source(44100 * 10);
    auto stream3 = device.create_stream(std::move(source3));
    stream3.open();
    stream3.play();
    bench.run("stream_set_volume", [&] {
        stream3.set_volume(0.5f);
    });
    
    bench.run("stream_get_volume", [&] {
        auto vol = stream3.volume();
        ankerl::nanobench::doNotOptimizeAway(vol);
    });
    
    // Benchmark: State queries
    bench.run("stream_is_playing", [&] {
        auto playing = stream3.is_playing();
        ankerl::nanobench::doNotOptimizeAway(playing);
    });
    
    bench.run("stream_is_paused", [&] {
        auto paused = stream3.is_paused();
        ankerl::nanobench::doNotOptimizeAway(paused);
    });
    
    // Cleanup
    stream3.stop();
}

} // namespace musac::benchmark