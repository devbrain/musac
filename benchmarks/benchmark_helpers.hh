#ifndef MUSAC_BENCHMARK_HELPERS_HH
#define MUSAC_BENCHMARK_HELPERS_HH

#include <musac/audio_source.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <memory>
#include <vector>

namespace musac::benchmark {

// Mock decoder for benchmarking
class benchmark_decoder : public decoder {
    size_t m_frames;
    size_t m_current_frame = 0;
    
public:
    explicit benchmark_decoder(size_t frames) : m_frames(frames) {}
    
    bool open(io_stream* /*stream*/) override { return true; }
    void close() override {}
    bool is_stereo() const override { return true; }
    int get_rate() const override { return 44100; }
    size_t decode(float* buffer, size_t size) override {
        size_t to_decode = std::min(size, m_frames - m_current_frame);
        for (size_t i = 0; i < to_decode; ++i) {
            buffer[i] = 0.1f;
        }
        m_current_frame += to_decode;
        return to_decode;
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

// Helper to create benchmark audio source
inline audio_source create_benchmark_source(size_t frames = 44100) {
    return audio_source(
        std::make_unique<benchmark_decoder>(frames),
        nullptr,
        false
    );
}

// Helper to create and setup audio device
inline audio_device setup_benchmark_device() {
    auto device = audio_device::open_default_device();
    device.resume();
    return device;
}

} // namespace musac::benchmark

#endif // MUSAC_BENCHMARK_HELPERS_HH