// Define before including headers to make audio_stream constructor public for benchmarking
#define MUSAC_BENCHMARK_MODE

#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <musac/audio_device_data.hh>
#include <musac/test_data/loader.hh>
#include <musac/sdk/audio_format.h>
#include <musac/sdk/types.h>
#include <musac/sdk/from_float_converter.hh>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <cstring>
#include <iomanip>
#include <numeric>
#include <algorithm>

using namespace musac;
using namespace std::chrono;

// Stats structure
struct timing_stats {
    double min_ms = std::numeric_limits<double>::max();
    double max_ms = 0.0;
    double avg_ms = 0.0;
    double median_ms = 0.0;
    double p95_ms = 0.0;
    double p99_ms = 0.0;
    uint64_t total_calls = 0;
    double total_time_ms = 0.0;
};

timing_stats calculate_stats(const std::vector<double>& times_ms) {
    timing_stats stats;
    
    if (times_ms.empty()) {
        return stats;
    }
    
    // Sort for percentiles
    std::vector<double> sorted_times = times_ms;
    std::sort(sorted_times.begin(), sorted_times.end());
    
    stats.min_ms = sorted_times.front();
    stats.max_ms = sorted_times.back();
    stats.total_calls = sorted_times.size();
    stats.total_time_ms = std::accumulate(sorted_times.begin(), sorted_times.end(), 0.0);
    stats.avg_ms = stats.total_time_ms / static_cast<double>(stats.total_calls);
    
    // Calculate median
    size_t mid = sorted_times.size() / 2;
    if (sorted_times.size() % 2 == 0) {
        stats.median_ms = (sorted_times[mid - 1] + sorted_times[mid]) / 2.0;
    } else {
        stats.median_ms = sorted_times[mid];
    }
    
    // Calculate percentiles
    size_t p95_idx = static_cast<size_t>(static_cast<double>(sorted_times.size()) * 0.95);
    size_t p99_idx = static_cast<size_t>(static_cast<double>(sorted_times.size()) * 0.99);
    stats.p95_ms = sorted_times[std::min(p95_idx, sorted_times.size() - 1)];
    stats.p99_ms = sorted_times[std::min(p99_idx, sorted_times.size() - 1)];
    
    return stats;
}

// Direct callback benchmark - tests raw audio_callback performance
void run_audio_callback_benchmark(const std::string& test_name, 
                                  test_data::music_type type,
                                  int num_streams,
                                  int buffer_size,
                                  int num_iterations) {
    std::cout << "\n========================================\n";
    std::cout << "Audio Callback Benchmark: " << test_name << "\n";
    std::cout << "Streams: " << num_streams << "\n";
    std::cout << "Buffer size: " << buffer_size << " bytes\n";
    std::cout << "Iterations: " << num_iterations << "\n";
    std::cout << "========================================\n";
    
    try {
        // Initialize test data
        test_data::loader::init();
        
        // Set up audio device data for the mixer
        audio_device_data dev_data;
        dev_data.m_audio_spec.format = audio_format::s16le;  // 16-bit signed little-endian
        dev_data.m_audio_spec.channels = 2;  // Stereo
        dev_data.m_audio_spec.freq = 44100;  // 44.1kHz
        // Set up the sample converter for float-to-s16le conversion
        dev_data.m_sample_converter = get_from_float_converter(audio_format::s16le);
        
        // Phase 1 optimization: Pre-calculate cached values for performance
        dev_data.m_bytes_per_sample = 2;  // 16-bit = 2 bytes
        dev_data.m_bytes_per_frame = dev_data.m_bytes_per_sample * dev_data.m_audio_spec.channels;
        dev_data.m_ms_per_frame = 1000.0f / static_cast<float>(dev_data.m_audio_spec.freq);
        dev_data.m_frame_size = 4096;  // Default frame size
        
        audio_stream::set_audio_device_data(dev_data);
        
        // Create streams
        std::vector<std::unique_ptr<audio_stream>> streams;
        for (int i = 0; i < num_streams; ++i) {
            // Load audio data
            auto src = test_data::loader::load(type);
            
            auto str = std::make_unique<audio_stream>(std::move(src));
            str->open();
            str->play();  // Start playback
            streams.push_back(std::move(str));
        }
        
        // Create buffer for audio callback
        std::vector<uint8> buffer(static_cast<size_t>(buffer_size));
        
        // Storage for timing measurements
        std::vector<double> callback_times_ms;
        callback_times_ms.reserve(static_cast<size_t>(num_iterations));
        
        // Warm-up phase (100 callbacks)
        std::cout << "Warming up...\n";
        for (int i = 0; i < 100; ++i) {
            // Clear buffer
            std::memset(buffer.data(), 0, buffer_size);
            
            // Call the static audio callback (it processes all streams internally)
            audio_stream::audio_callback(buffer.data(), static_cast<unsigned int>(buffer_size));
        }
        
        // Actual benchmark
        std::cout << "Running benchmark (" << num_iterations << " iterations)...\n";
        auto bench_start = high_resolution_clock::now();
        
        for (int i = 0; i < num_iterations; ++i) {
            // Clear buffer
            std::memset(buffer.data(), 0, buffer_size);
            
            // Measure callback time
            auto callback_start = high_resolution_clock::now();
            
            // Call the static audio callback (it processes all streams internally via mixer)
            audio_stream::audio_callback(buffer.data(), static_cast<unsigned int>(buffer_size));
            
            auto callback_end = high_resolution_clock::now();
            double callback_time_ms = static_cast<double>(duration_cast<nanoseconds>(callback_end - callback_start).count()) / 1000000.0;
            callback_times_ms.push_back(callback_time_ms);
            
            // Progress indicator
            if (i % 1000 == 0 && i > 0) {
                std::cout << "." << std::flush;
            }
        }
        
        auto bench_end = high_resolution_clock::now();
        double total_bench_time_ms = static_cast<double>(duration_cast<microseconds>(bench_end - bench_start).count()) / 1000.0;
        
        // Calculate and print stats
        auto stats = calculate_stats(callback_times_ms);
        
        std::cout << "\n\nResults:\n";
        std::cout << "--------\n";
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Total callbacks: " << stats.total_calls << "\n";
        std::cout << "Total benchmark time: " << total_bench_time_ms << " ms\n";
        std::cout << "Total callback time: " << stats.total_time_ms << " ms\n";
        std::cout << "Callback overhead: " << (stats.total_time_ms / total_bench_time_ms * 100.0) << "%\n\n";
        
        std::cout << "Per-callback timing:\n";
        std::cout << "  Min: " << stats.min_ms << " ms\n";
        std::cout << "  Max: " << stats.max_ms << " ms\n";
        std::cout << "  Avg: " << stats.avg_ms << " ms\n";
        std::cout << "  Median: " << stats.median_ms << " ms\n";
        std::cout << "  95th percentile: " << stats.p95_ms << " ms\n";
        std::cout << "  99th percentile: " << stats.p99_ms << " ms\n";
        
        // Calculate real-time safety
        double samples_per_buffer = static_cast<double>(buffer_size) / 4.0;  // Assuming 16-bit stereo
        double buffer_duration_ms = (samples_per_buffer / 44.1);  // 44.1kHz sample rate
        
        std::cout << "\nReal-time analysis:\n";
        std::cout << "  Expected buffer duration: " << buffer_duration_ms << " ms\n";
        std::cout << "  Safety margin (99th percentile): " << (buffer_duration_ms - stats.p99_ms) << " ms\n";
        
        if (stats.p99_ms > buffer_duration_ms) {
            std::cout << "  WARNING: 99th percentile exceeds real-time deadline!\n";
        } else {
            std::cout << "  OK: Meets real-time requirements\n";
        }
        
        // Throughput analysis
        double total_data_mb = (static_cast<double>(buffer_size) * static_cast<double>(num_iterations) * static_cast<double>(num_streams)) / (1024.0 * 1024.0);
        double throughput_mbps = total_data_mb / (total_bench_time_ms / 1000.0);
        
        std::cout << "\nThroughput:\n";
        std::cout << "  Total data processed: " << total_data_mb << " MB\n";
        std::cout << "  Processing rate: " << throughput_mbps << " MB/s\n";
        
        // Per-stream throughput
        double per_stream_mbps = throughput_mbps / static_cast<double>(num_streams);
        std::cout << "  Per-stream rate: " << per_stream_mbps << " MB/s\n";
        
        // CSV output for this run
        std::cout << "\nCSV: " << test_name << "," << num_streams << "," << buffer_size 
                  << "," << stats.min_ms << "," << stats.max_ms << "," << stats.avg_ms 
                  << "," << stats.median_ms << "," << stats.p95_ms << "," << stats.p99_ms << "\n";
        
        // Cleanup
        test_data::loader::done();
        
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << "\n";
        test_data::loader::done();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Audio Callback Performance Baseline Benchmark\n";
    std::cout << "==============================================\n";
    std::cout << "This benchmark establishes baseline performance metrics\n";
    std::cout << "for the audio callback before optimization.\n\n";
    
    // Parse command line arguments
    int num_iterations = 10000;
    if (argc > 1) {
        num_iterations = std::atoi(argv[1]);
    }
    
    // Test cases
    struct test_case {
        std::string name;
        test_data::music_type type;
    };
    
    std::vector<test_case> test_cases = {
        {"MP3", test_data::music_type::mp3},
        {"VOC", test_data::music_type::voc},
        {"S3M", test_data::music_type::s3m},
        {"MID", test_data::music_type::mid}
    };
    
    // Buffer sizes to test (in bytes)
    std::vector<int> buffer_sizes = {512, 1024, 2048, 4096};
    
    // Stream counts to test
    std::vector<int> stream_counts = {1, 2, 4, 8};
    
    // Write CSV header for easy analysis
    std::cout << "\n=== CSV OUTPUT ===\n";
    std::cout << "Format,Streams,BufferSize,Min_ms,Max_ms,Avg_ms,Median_ms,P95_ms,P99_ms\n";
    
    // Run baseline single stream tests first
    std::cout << "\n=== BASELINE SINGLE STREAM ===\n";
    for (const auto& test : test_cases) {
        for (int buffer_size : buffer_sizes) {
            run_audio_callback_benchmark(test.name + "_Baseline", 
                                        test.type, 
                                        1,  // Single stream
                                        buffer_size, 
                                        num_iterations);
        }
    }
    
    // Run multi-stream benchmarks
    std::cout << "\n\n=== MULTI-STREAM BENCHMARKS ===\n";
    for (const auto& test : test_cases) {
        for (int buffer_size : buffer_sizes) {
            for (int num_streams : stream_counts) {
                if (num_streams > 1) {  // Skip 1 since we did baseline
                    run_audio_callback_benchmark(test.name, 
                                                test.type, 
                                                num_streams, 
                                                buffer_size, 
                                                num_iterations);
                }
            }
        }
    }
    
    std::cout << "\n\n=== BASELINE BENCHMARK COMPLETE ===\n";
    std::cout << "Save these results before starting optimization.\n";
    std::cout << "Run with: ./build/bin/standalone_audio_callback_benchmark [iterations]\n";
    std::cout << "Default iterations: 10000\n";
    std::cout << "\nTo save results: ./build/bin/standalone_audio_callback_benchmark > baseline_results.txt\n";
    
    return 0;
}