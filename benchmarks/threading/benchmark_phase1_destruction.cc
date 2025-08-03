#include <nanobench.h>
#include "../benchmark_helpers.hh"

namespace musac::benchmark {

void register_phase1_destruction_benchmarks(ankerl::nanobench::Bench& bench) {
    bench.title("Phase 1 - Stream Destruction");
    
    auto device = setup_benchmark_device();
    
    // Benchmark: Stream destruction timing
    bench.run("stream_destruction_idle", [&] {
        auto source = create_benchmark_source(44100);
        auto stream = device.create_stream(std::move(source));
        stream.open();
        // Destruction happens here
    });
    
    bench.run("stream_destruction_playing", [&] {
        auto source = create_benchmark_source(44100);
        auto stream = device.create_stream(std::move(source));
        stream.open();
        stream.play();
        // Destruction while playing
    });
}

} // namespace musac::benchmark