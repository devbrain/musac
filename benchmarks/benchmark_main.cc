#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>
#include <musac/audio_system.hh>
#include <iostream>

// Declare benchmark suites
namespace musac::benchmark {
    void register_stream_operation_benchmarks(ankerl::nanobench::Bench& bench);
    void register_mixer_operation_benchmarks(ankerl::nanobench::Bench& bench);
    void register_lock_contention_benchmarks(ankerl::nanobench::Bench& bench);
    void register_phase1_destruction_benchmarks(ankerl::nanobench::Bench& bench);
    void register_phase2_locking_benchmarks(ankerl::nanobench::Bench& bench);
}

int main(int argc, char** argv) {
    // Initialize audio system
    musac::audio_system::init();
    
    // Create benchmark instance
    ankerl::nanobench::Bench bench;
    bench.title("Musac Audio Library Benchmarks");
    bench.relative(true);
    bench.performanceCounters(true);
    
    std::cout << "Running Musac benchmarks...\n\n";
    
    // Register all benchmark suites
    musac::benchmark::register_stream_operation_benchmarks(bench);
    musac::benchmark::register_mixer_operation_benchmarks(bench);
    musac::benchmark::register_lock_contention_benchmarks(bench);
    musac::benchmark::register_phase1_destruction_benchmarks(bench);
    musac::benchmark::register_phase2_locking_benchmarks(bench);
    
    // Cleanup
    musac::audio_system::done();
    
    return 0;
}