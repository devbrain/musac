#ifndef MUSAC_MOCK_COMPONENTS_HH
#define MUSAC_MOCK_COMPONENTS_HH

#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/sdk/io_stream.hh>
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
    size_t read(void* ptr, size_t size_bytes) override {
        if (!m_is_open) return 0;
        
        size_t available = m_data.size() - m_position;
        size_t to_read = std::min(available, size_bytes);
        
        if (to_read > 0) {
            std::memcpy(ptr, m_data.data() + m_position, to_read);
            m_position += to_read;
        }
        
        return to_read;
    }
    
    size_t write(const void* ptr, size_t size_bytes) override {
        if (!m_is_open) return 0;
        
        m_data.resize(m_position + size_bytes);
        std::memcpy(m_data.data() + m_position, ptr, size_bytes);
        m_position += size_bytes;
        return size_bytes;
    }
    
    int64_t seek(int64_t offset, musac::seek_origin whence) override {
        if (!m_is_open) return -1;
        
        int64_t new_pos = static_cast<int64_t>(m_position);
        
        switch (whence) {
            case musac::seek_origin::set:
                new_pos = offset;
                break;
            case musac::seek_origin::cur:
                new_pos = static_cast<int64_t>(m_position) + offset;
                break;
            case musac::seek_origin::end:
                new_pos = static_cast<int64_t>(m_data.size()) + offset;
                break;
        }
        
        if (new_pos < 0 || new_pos > static_cast<int64_t>(m_data.size())) {
            return -1;
        }
        
        m_position = static_cast<size_t>(new_pos);
        return new_pos;
    }
    
    int64_t tell() override {
        return m_is_open ? static_cast<int64_t>(m_position) : -1;
    }
    
    int64_t get_size() override {
        return m_is_open ? static_cast<int64_t>(m_data.size()) : -1;
    }
    
    void close() override {
        m_is_open = false;
    }
    
    bool is_open() const override {
        return m_is_open;
    }
    
private:
    std::vector<uint8_t> m_data;
    size_t m_position;
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
    
    test_decoder(size_t total_frames = 44100, Pattern pattern = Pattern::SILENCE) 
        : decoder(),
          m_current_frame(0),
          m_total_frames(total_frames),
          m_pattern(pattern),
          m_channels(2),
          m_sample_rate(44100) {}
    
    void open(musac::io_stream* /*rwops*/) override {
        set_is_open(true);
    }
    
    channels_t get_channels() const override {
        return m_channels;
    }
    
    sample_rate_t get_rate() const override {
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
        size_t frame_pos = static_cast<size_t>((pos.count() * m_sample_rate) / 1000000);
        if (frame_pos < m_total_frames) {
            m_current_frame = frame_pos;
            return true;
        }
        return false;
    }
    
    const char* get_name() const override {
        return "Test Decoder";
    }
    
protected:

    size_t do_decode(float* buf, size_t len, bool& call_again) override {
        size_t frames_requested = len / m_channels;
        size_t frames_to_read = std::min(frames_requested, 
            m_total_frames - m_current_frame);
        size_t samples_to_read = frames_to_read * m_channels;
        
        switch (m_pattern) {
            case Pattern::SILENCE:
                for (size_t i = 0; i < samples_to_read; ++i) {
                    buf[i] = 0.0f;
                }
                break;
                
            case Pattern::SINE_440HZ:
                for (size_t i = 0; i < frames_to_read; ++i) {
                    float sample = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * static_cast<double>(m_current_frame + i) / static_cast<double>(m_sample_rate)) * 0.3);
                    for (channels_t ch = 0; ch < m_channels; ++ch) {
                        buf[i * m_channels + ch] = sample;
                    }
                }
                break;
                
            case Pattern::WHITE_NOISE:
                for (size_t i = 0; i < samples_to_read; ++i) {
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
    size_t get_read_count() const { return m_read_count; }
    size_t get_current_frame() const { return m_current_frame; }
    size_t get_rewind_count() const { return m_rewind_count; }
    
    void set_channels(channels_t channels) { m_channels = channels; }
    void set_sample_rate(sample_rate_t rate) { m_sample_rate = rate; }
    
protected:
    size_t m_current_frame;
    
private:
    size_t m_total_frames;
    Pattern m_pattern;
    channels_t m_channels;
    sample_rate_t m_sample_rate;
    size_t m_read_count = 0;
    size_t m_rewind_count = 0;
    
};

// Shared state for mock_audio_source that survives moves
struct mock_audio_state {
    size_t total_frames;
    std::atomic<size_t> current_frame{0};
    std::atomic<size_t> rewind_count{0};
    std::atomic<size_t> read_count{0};
    std::atomic<size_t> open_count{0};
    std::atomic<sample_rate_t> rate{44100};
    std::atomic<channels_t> channels{2};
    std::atomic<size_t> frame_size{0};
    std::atomic<bool> is_open{false};
    bool generate_sine = false;
    
    explicit mock_audio_state(size_t frames) : total_frames(frames) {}
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
    static std::unique_ptr<mock_audio_source> create(size_t total_frames = 44100);
    
    void open(sample_rate_t rate, channels_t channels, size_t frame_size) override {
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
        size_t frame_pos = static_cast<size_t>((pos.count() * m_state->rate) / 1000000);
        if (frame_pos < m_state->total_frames) {
            m_state->current_frame = frame_pos;
            return true;
        }
        return false;
    }
    
    std::chrono::microseconds duration() const override {
        return std::chrono::microseconds((m_state->total_frames * 1000000) / m_state->rate);
    }
    
    void read_samples(float* out, size_t& cur_pos, size_t out_len, channels_t out_channels) override {
        size_t frames_requested = (out_len - cur_pos) / out_channels;
        size_t frames_to_read = std::min(frames_requested, 
            m_state->total_frames - m_state->current_frame);
        
        // Generate simple sine wave or silence based on pattern
        for (size_t i = 0; i < frames_to_read; ++i) {
            float sample = 0.0f;
            if (m_state->generate_sine) {
                sample = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * static_cast<double>(m_state->current_frame + i) / static_cast<double>(m_state->rate)) * 0.3);
            }
            
            for (channels_t ch = 0; ch < out_channels; ++ch) {
                out[cur_pos++] = sample;
            }
        }
        
        m_state->current_frame += frames_to_read;
        m_state->read_count++;
    }
    
    // Test control methods
    void set_generate_sine(bool generate) { m_state->generate_sine = generate; }
    
    // Test inspection methods  
    size_t get_rewind_count() const { return m_state->rewind_count; }
    size_t get_current_frame() const { return m_state->current_frame; }
    size_t get_read_count() const { return m_state->read_count; }
    size_t get_open_count() const { return m_state->open_count; }
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
    size_t do_decode(float* buf, size_t len, bool& call_again) override {
        size_t result = test_decoder::do_decode(buf, len, call_again);
        m_state->read_count++;
        m_state->current_frame = m_current_frame;
        return result;
    }
    
private:
    std::shared_ptr<mock_audio_state> m_state;
};

// Factory method implementation
inline std::unique_ptr<mock_audio_source> mock_audio_source::create(size_t total_frames) {
    auto state = std::make_shared<mock_audio_state>(total_frames);
    auto decoder = std::make_unique<test_decoder_with_state>(state);
    auto io_stream = std::make_unique<memory_io_stream>();
    
    return std::unique_ptr<mock_audio_source>(
        new mock_audio_source(std::move(decoder), std::move(io_stream), state)
    );
}

// Helper factory functions
inline std::unique_ptr<audio_source> create_test_source(
    size_t frames = 44100, 
    test_decoder::Pattern pattern = test_decoder::Pattern::SILENCE) {
    
    auto decoder = std::make_unique<test_decoder>(frames, pattern);
    auto io = std::make_unique<memory_io_stream>();
    
    return std::make_unique<audio_source>(std::move(decoder), std::move(io));
}

inline std::unique_ptr<mock_audio_source> create_mock_source(size_t frames = 44100) {
    return mock_audio_source::create(frames);
}

// Mock decoder that can simulate errors
class mock_decoder_with_errors : public decoder {
private:
    channels_t m_channels{2};
    sample_rate_t m_rate{44100};
    size_t m_total_samples;
    size_t m_current_sample{0};
    bool m_is_open{false};
    
public:
    // Error injection flags
    bool fail_on_decode{false};
    bool wrong_format{false};
    bool fail_on_seek{false};
    bool return_partial_data{false};
    
    mock_decoder_with_errors() : m_total_samples(44100) {} // 1 second default
    
    void open(io_stream* stream) override {
        (void)stream; // Unused
        m_is_open = true;
        set_is_open(true);
    }
    
    channels_t get_channels() const override {
        return m_channels;
    }
    
    sample_rate_t get_rate() const override {
        return m_rate;
    }
    
    bool rewind() override {
        if (fail_on_seek) {
            return false;
        }
        m_current_sample = 0;
        return true;
    }
    
    std::chrono::microseconds duration() const override {
        double duration_secs = static_cast<double>(m_total_samples) / m_rate;
        return std::chrono::microseconds(static_cast<long long>(duration_secs * 1000000));
    }
    
    bool seek_to_time(std::chrono::microseconds pos) override {
        if (fail_on_seek) {
            return false;
        }
        
        double position_secs = static_cast<double>(pos.count()) / 1000000.0;
        size_t target_sample = static_cast<size_t>(position_secs * m_rate);
        if (target_sample <= m_total_samples) {
            m_current_sample = target_sample;
            return true;
        }
        return false;
    }
    
    const char* get_name() const override {
        return "Mock Decoder With Errors";
    }
    
protected:

    size_t do_decode(float* buf, size_t len, bool& call_again) override {
        if (fail_on_decode) {
            throw std::runtime_error("Simulated decode failure");
        }
        
        size_t samples_to_generate = len;
        
        if (return_partial_data) {
            samples_to_generate /= 2; // Return only half
        }
        
        // Don't exceed total samples
        size_t remaining = m_total_samples - m_current_sample;
        samples_to_generate = std::min(remaining, samples_to_generate);
        
        if (samples_to_generate == 0) {
            call_again = false;
            return 0; // End of stream
        }
        
        // Generate corrupted or normal data
        if (buf) {
            if (fail_on_decode) {
                // Write random floats
                for (size_t i = 0; i < samples_to_generate; ++i) {
                    buf[i] = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
                }
            } else {
                // Write zeros (silence)
                std::memset(buf, 0, samples_to_generate * sizeof(float));
            }
        }
        
        m_current_sample += samples_to_generate;
        call_again = (m_current_sample < m_total_samples);
        return samples_to_generate;
    }
};

// Mock IO stream that can simulate errors  
class mock_io_stream : public io_stream {
private:
    std::vector<uint8_t> m_data;
    size_t m_position{0};
    bool m_is_open{true};
    
public:
    // Error injection flags
    bool fail_on_read{false};
    bool fail_on_seek{false};
    bool return_partial_reads{false};
    
    mock_io_stream(size_t data_size = 1024) : m_data(data_size, 0) {}
    
    size_t read(void* buffer, size_t bytes) override {
        if (fail_on_read) {
            return 0;
        }
        
        size_t available = m_data.size() - m_position;
        size_t to_read = std::min(bytes, available);
        
        if (return_partial_reads && to_read > 0) {
            to_read /= 2;
        }
        
        if (buffer && to_read > 0) {
            std::memcpy(buffer, m_data.data() + m_position, to_read);
            m_position += to_read;
        }
        
        return to_read;
    }
    
    size_t write(const void* buffer, size_t bytes) override {
        if (m_position + bytes > m_data.size()) {
            m_data.resize(m_position + bytes);
        }
        
        if (buffer) {
            std::memcpy(m_data.data() + m_position, buffer, bytes);
            m_position += bytes;
        }
        
        return bytes;
    }
    
    int64_t seek(int64_t offset, seek_origin whence) override {
        if (fail_on_seek) {
            return -1;
        }
        
        size_t new_position;
        switch (whence) {
            case seek_origin::set:
                if (offset < 0) return -1;
                new_position = static_cast<size_t>(offset);
                break;
            case seek_origin::cur:
                if (offset < 0 && static_cast<size_t>(-offset) > m_position) {
                    return -1;
                }
                new_position = m_position + static_cast<size_t>(offset);
                break;
            case seek_origin::end:
                if (offset > 0) return -1;
                if (static_cast<size_t>(-offset) > m_data.size()) {
                    return -1;
                }
                new_position = m_data.size() + static_cast<size_t>(offset);
                break;
            default:
                return -1;
        }
        
        if (new_position > m_data.size()) {
            return -1;
        }
        
        m_position = new_position;
        return static_cast<int64_t>(m_position);
    }
    
    int64_t tell() override {
        return static_cast<int64_t>(m_position);
    }
    
    int64_t get_size() override {
        return static_cast<int64_t>(m_data.size());
    }
    
    void close() override {
        m_is_open = false;
    }
    
    bool is_open() const override {
        return m_is_open;
    }
};

} // namespace musac::test

#endif // MUSAC_MOCK_COMPONENTS_HH