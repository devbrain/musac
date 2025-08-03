#ifndef MUSAC_TEST_HELPERS_HH
#define MUSAC_TEST_HELPERS_HH

#include <musac/sdk/decoder.hh>
#include <musac/sdk/io_stream.h>
#include <musac/sdk/types.h>
#include <musac/audio_source.hh>
#include <cstring>
#include <vector>
#include <cmath>
#include <atomic>
#include <algorithm>
#include <cstdlib>

namespace musac::test {

// Test implementation of io_stream that works with memory buffers
class memory_io_stream : public musac::io_stream {
public:
    memory_io_stream(const std::vector<uint8_t>& data) 
        : m_data(data), m_position(0), m_is_open(true) {}
    
    memory_io_stream() : m_position(0), m_is_open(true) {}
    
    // io_stream implementation
    musac::size read(void* ptr, musac::size size_bytes) override {
        if (!m_is_open) return 0;
        
        musac::size available = m_data.size() - m_position;
        musac::size to_read = std::min(available, size_bytes);
        
        if (to_read > 0) {
            std::memcpy(ptr, m_data.data() + m_position, to_read);
            m_position += to_read;
        }
        
        return to_read;
    }
    
    musac::size write(const void* ptr, musac::size size_bytes) override {
        if (!m_is_open) return 0;
        
        m_data.resize(m_position + size_bytes);
        std::memcpy(m_data.data() + m_position, ptr, size_bytes);
        m_position += size_bytes;
        return size_bytes;
    }
    
    musac::int64 seek(musac::int64 offset, musac::seek_origin whence) override {
        if (!m_is_open) return -1;
        
        musac::int64 new_pos = static_cast<musac::int64>(m_position);
        
        switch (whence) {
            case musac::seek_origin::set:
                new_pos = offset;
                break;
            case musac::seek_origin::cur:
                new_pos = static_cast<musac::int64>(m_position) + offset;
                break;
            case musac::seek_origin::end:
                new_pos = static_cast<musac::int64>(m_data.size()) + offset;
                break;
        }
        
        if (new_pos < 0 || new_pos > static_cast<musac::int64>(m_data.size())) {
            return -1;
        }
        
        m_position = static_cast<musac::size>(new_pos);
        return new_pos;
    }
    
    musac::int64 tell() override {
        return m_is_open ? static_cast<musac::int64>(m_position) : -1;
    }
    
    musac::int64 get_size() override {
        return m_is_open ? static_cast<musac::int64>(m_data.size()) : -1;
    }
    
    void close() override {
        m_is_open = false;
    }
    
    bool is_open() const override {
        return m_is_open;
    }
    
private:
    std::vector<uint8_t> m_data;
    musac::size m_position;
    bool m_is_open;
};

// Test decoder that generates silence or test patterns
class test_decoder : public decoder {
public:
    enum class Pattern {
        SILENCE,
        SINE_440HZ,
        WHITE_NOISE
    };
    
    test_decoder(musac::size total_frames = 44100, Pattern pattern = Pattern::SILENCE) 
        : decoder(),
          m_current_frame(0),
          m_total_frames(total_frames),
          m_pattern(pattern),
          m_channels(2),
          m_sample_rate(44100) {}
    
    void open(musac::io_stream* /*rwops*/) override {
        set_is_open(true);
    }
    
    unsigned int get_channels() const override {
        return m_channels;
    }
    
    unsigned int get_rate() const override {
        return m_sample_rate;
    }
    
    bool rewind() override {
        m_current_frame = 0;
        m_rewind_count++;
        return true;
    }
    
    std::chrono::microseconds duration() const override {
        return std::chrono::microseconds((m_total_frames * 1000000) / m_sample_rate);
    }
    
    bool seek_to_time(std::chrono::microseconds pos) override {
        musac::size frame_pos = static_cast<musac::size>((pos.count() * m_sample_rate) / 1000000);
        if (frame_pos < m_total_frames) {
            m_current_frame = frame_pos;
            return true;
        }
        return false;
    }
    
protected:
    unsigned int do_decode(float* buf, unsigned int len, bool& call_again) override {
        unsigned int frames_requested = len / m_channels;
        unsigned int frames_to_read = std::min(frames_requested, 
            static_cast<unsigned int>(m_total_frames - m_current_frame));
        unsigned int samples_to_read = frames_to_read * m_channels;
        
        switch (m_pattern) {
            case Pattern::SILENCE:
                for (unsigned int i = 0; i < samples_to_read; ++i) {
                    buf[i] = 0.0f;
                }
                break;
                
            case Pattern::SINE_440HZ:
                for (unsigned int i = 0; i < frames_to_read; ++i) {
                    float sample = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * static_cast<double>(m_current_frame + i) / static_cast<double>(m_sample_rate)) * 0.3);
                    for (unsigned int ch = 0; ch < m_channels; ++ch) {
                        buf[i * m_channels + ch] = sample;
                    }
                }
                break;
                
            case Pattern::WHITE_NOISE:
                for (unsigned int i = 0; i < samples_to_read; ++i) {
                    buf[i] = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 0.3f;
                }
                break;
        }
        
        m_current_frame += frames_to_read;
        m_read_count++;
        
        call_again = (m_current_frame < m_total_frames);
        return samples_to_read;
    }
    
public:
    // Test helpers
    musac::size get_read_count() const { return m_read_count; }
    musac::size get_current_frame() const { return m_current_frame; }
    musac::size get_rewind_count() const { return m_rewind_count; }
    
    void set_channels(unsigned int channels) { m_channels = channels; }
    void set_sample_rate(unsigned int rate) { m_sample_rate = rate; }
    
protected:
    musac::size m_current_frame;
    
private:
    musac::size m_total_frames;
    Pattern m_pattern;
    unsigned int m_channels;
    unsigned int m_sample_rate;
    musac::size m_read_count = 0;
    musac::size m_rewind_count = 0;
    
};

// Shared state for mock_audio_source that survives moves
struct mock_audio_state {
    musac::size total_frames;
    std::atomic<musac::size> current_frame{0};
    std::atomic<musac::size> rewind_count{0};
    std::atomic<musac::size> read_count{0};
    std::atomic<musac::size> open_count{0};
    std::atomic<unsigned int> rate{44100};
    std::atomic<unsigned int> channels{2};
    std::atomic<unsigned int> frame_size{0};
    std::atomic<bool> is_open{false};
    bool generate_sine = false;
    
    explicit mock_audio_state(musac::size frames) : total_frames(frames) {}
};

// Mock audio source for testing - directly overrides virtual methods
class mock_audio_source : public audio_source {
private:
    // Private constructor for factory
    mock_audio_source(std::unique_ptr<decoder> dec, std::unique_ptr<musac::io_stream> io, 
                      std::shared_ptr<mock_audio_state> state)
        : audio_source(std::move(dec), std::move(io)), m_state(state) {}
        
public:
    // Factory method
    static std::unique_ptr<mock_audio_source> create(musac::size total_frames = 44100);
    
    void open(unsigned int rate, unsigned int channels, unsigned int frame_size) override {
        m_state->rate = rate;
        m_state->channels = channels;
        m_state->frame_size = frame_size;
        m_state->is_open = true;
        m_state->open_count++;
    }
    
    bool rewind() override {
        m_state->current_frame = 0;
        m_state->rewind_count++;
        return true;
    }
    
    bool seek_to_time(std::chrono::microseconds pos) const override {
        musac::size frame_pos = static_cast<musac::size>((pos.count() * m_state->rate) / 1000000);
        if (frame_pos < m_state->total_frames) {
            m_state->current_frame = frame_pos;
            return true;
        }
        return false;
    }
    
    std::chrono::microseconds duration() const override {
        return std::chrono::microseconds((m_state->total_frames * 1000000) / m_state->rate);
    }
    
    void read_samples(float* out, unsigned int& cur_pos, unsigned int out_len, unsigned int out_channels) override {
        unsigned int frames_requested = (out_len - cur_pos) / out_channels;
        unsigned int frames_to_read = std::min(frames_requested, 
            static_cast<unsigned int>(m_state->total_frames - m_state->current_frame));
        
        // Generate simple sine wave or silence based on pattern
        for (unsigned int i = 0; i < frames_to_read; ++i) {
            float sample = 0.0f;
            if (m_state->generate_sine) {
                sample = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * static_cast<double>(m_state->current_frame + i) / static_cast<double>(m_state->rate)) * 0.3);
            }
            
            for (unsigned int ch = 0; ch < out_channels; ++ch) {
                out[cur_pos++] = sample;
            }
        }
        
        m_state->current_frame += frames_to_read;
        m_state->read_count++;
    }
    
    // Test control methods
    void set_generate_sine(bool generate) { m_state->generate_sine = generate; }
    
    // Test inspection methods  
    musac::size get_rewind_count() const { return m_state->rewind_count; }
    musac::size get_current_frame() const { return m_state->current_frame; }
    musac::size get_read_count() const { return m_state->read_count; }
    musac::size get_open_count() const { return m_state->open_count; }
    bool is_open() const { return m_state->is_open; }
    
    // Get the shared state for tests
    std::shared_ptr<mock_audio_state> get_state() const { return m_state; }
    
private:
    std::shared_ptr<mock_audio_state> m_state;
};

// Test decoder that shares state with mock_audio_source
class test_decoder_with_state : public test_decoder {
public:
    test_decoder_with_state(std::shared_ptr<mock_audio_state> state)
        : test_decoder(state->total_frames, Pattern::SILENCE), m_state(state) {}
    
    bool rewind() override {
        m_current_frame = 0;
        m_state->current_frame = 0;
        m_state->rewind_count++;
        return true;
    }
    
    bool seek_to_time(std::chrono::microseconds pos) override {
        bool result = test_decoder::seek_to_time(pos);
        if (result) {
            m_state->current_frame = m_current_frame;
        }
        return result;
    }
    
protected:
    unsigned int do_decode(float* buf, unsigned int len, bool& call_again) override {
        unsigned int result = test_decoder::do_decode(buf, len, call_again);
        m_state->read_count++;
        m_state->current_frame = m_current_frame;
        return result;
    }
    
private:
    std::shared_ptr<mock_audio_state> m_state;
};

// Factory method implementation
inline std::unique_ptr<mock_audio_source> mock_audio_source::create(musac::size total_frames) {
    auto state = std::make_shared<mock_audio_state>(total_frames);
    auto decoder = std::make_unique<test_decoder_with_state>(state);
    auto io_stream = std::make_unique<memory_io_stream>();
    
    return std::unique_ptr<mock_audio_source>(
        new mock_audio_source(std::move(decoder), std::move(io_stream), state)
    );
}

// Helper factory functions
inline std::unique_ptr<audio_source> create_test_source(
    musac::size frames = 44100, 
    test_decoder::Pattern pattern = test_decoder::Pattern::SILENCE) {
    
    auto decoder = std::make_unique<test_decoder>(frames, pattern);
    auto io = std::make_unique<memory_io_stream>();
    
    return std::make_unique<audio_source>(std::move(decoder), std::move(io));
}

inline std::unique_ptr<mock_audio_source> create_mock_source(musac::size frames = 44100) {
    return mock_audio_source::create(frames);
}

} // namespace musac::test

#endif // MUSAC_TEST_HELPERS_HH