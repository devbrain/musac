#ifndef MUSAC_BENCHMARK_HELPERS_HH
#define MUSAC_BENCHMARK_HELPERS_HH

#include <musac/audio_source.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/sdk/io_stream.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

namespace musac::benchmark {

// Mock decoder for benchmarking
class benchmark_decoder : public decoder {
    size_t m_frames;
    size_t m_current_frame = 0;
    
public:
    explicit benchmark_decoder(size_t frames) : m_frames(frames) {}
    
    void open(io_stream* /*stream*/) override { /* no-op for benchmark */ }
    unsigned int get_channels() const override { return 2; }
    unsigned int get_rate() const override { return 44100; }
    unsigned int do_decode(float* buf, unsigned int len, bool& call_again) override {
        unsigned int frames_requested = len / 2; // stereo
        unsigned int frames_to_decode = std::min(static_cast<unsigned int>(frames_requested), 
                                                  static_cast<unsigned int>(m_frames - m_current_frame));
        unsigned int samples = frames_to_decode * 2;
        
        for (unsigned int i = 0; i < samples; ++i) {
            buf[i] = 0.1f;
        }
        
        m_current_frame += frames_to_decode;
        call_again = (m_current_frame < m_frames);
        return samples;
    }
    bool rewind() override { 
        m_current_frame = 0;
        return true; 
    }
    std::chrono::microseconds duration() const override {
        return std::chrono::microseconds(m_frames * 1000000 / 44100);
    }
    bool seek_to_time(std::chrono::microseconds /*pos*/) override { return false; }
};

// Dummy io_stream for benchmarking
class benchmark_io_stream : public io_stream {
public:
    size read(void* /*ptr*/, size /*size*/) override { return 0; }
    size write(const void* /*ptr*/, size /*size*/) override { return 0; }
    int64 seek(int64 /*offset*/, seek_origin /*whence*/) override { return 0; }
    int64 tell() override { return 0; }
    int64 get_size() override { return 0; }
    void close() override {}
    bool is_open() const override { return true; }
};

// Helper to create benchmark audio source
inline audio_source create_benchmark_source(size_t frames = 44100) {
    return audio_source(
        std::make_unique<benchmark_decoder>(frames),
        std::make_unique<benchmark_io_stream>()
    );
}

// Helper to create and setup audio device
inline audio_device setup_benchmark_device() {
    auto device = audio_device::open_default_device();
    device.resume();
    return device;
}

// Helper to ensure clean shutdown of audio device
inline void cleanup_benchmark_device(audio_device& device) {
    // Pause the device to stop callbacks
    device.pause();
    
    // Give more time for any running callbacks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // The device destructor will handle the rest
}

// RAII wrapper for benchmark device that ensures proper cleanup
class benchmark_device_guard {
    audio_device m_device;
    
public:
    benchmark_device_guard() : m_device(setup_benchmark_device()) {}
    
    ~benchmark_device_guard() {
        cleanup_benchmark_device(m_device);
    }
    
    audio_device& get() { return m_device; }
    const audio_device& get() const { return m_device; }
    
    // Disable copy
    benchmark_device_guard(const benchmark_device_guard&) = delete;
    benchmark_device_guard& operator=(const benchmark_device_guard&) = delete;
    
    // Enable move
    benchmark_device_guard(benchmark_device_guard&&) = default;
    benchmark_device_guard& operator=(benchmark_device_guard&&) = default;
};

} // namespace musac::benchmark

#endif // MUSAC_BENCHMARK_HELPERS_HH