/**
 * @file decoder_8svx.cc
 * @brief IFF 8SVX (8-bit Sampled Voice) decoder implementation
 */

#include <musac/codecs/decoder_8svx.hh>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/audio_converter.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>

#include <iff/parser.hh>
#include <iff/handler_registry.hh>
#include <iff/chunk_reader.hh>
#include <iff/fourcc.hh>

#include <algorithm>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream>

namespace musac {

// 8SVX chunk identifiers using iff::fourcc
static const iff::fourcc FORM_ID("FORM");
static const iff::fourcc ESVX_ID("8SVX");  // Note: "8SVX" as fourcc
static const iff::fourcc VHDR_ID("VHDR");
static const iff::fourcc BODY_ID("BODY");
static const iff::fourcc NAME_ID("NAME");
static const iff::fourcc AUTH_ID("AUTH");
static const iff::fourcc COPY_ID("(c) ");
static const iff::fourcc ANNO_ID("ANNO");
static const iff::fourcc ATAK_ID("ATAK");
static const iff::fourcc RLSE_ID("RLSE");

// Compression types
enum SVXCompression : uint8_t {
    COMP_NONE = 0,        // No compression
    COMP_FIB_DELTA = 1    // Fibonacci-delta encoding
};

// Helper to swap endianness
template<typename T>
T swap_endian(T value) {
    union {
        T value;
        uint8_t bytes[sizeof(T)];
    } src, dst;
    
    src.value = value;
    for (size_t i = 0; i < sizeof(T); ++i) {
        dst.bytes[i] = src.bytes[sizeof(T) - 1 - i];
    }
    return dst.value;
}

// Fibonacci-delta decompression
class FibonacciDeltaDecoder {
    static constexpr int8_t fib_table[16] = {
        -34, -21, -13, -8, -5, -3, -2, -1,
        0, 1, 2, 3, 5, 8, 13, 21
    };
    
    int8_t current_value = 0;
    
public:
    void reset() {
        current_value = 0;
    }
    
    void decode(const uint8_t* src, int8_t* dst, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            uint8_t byte = src[i];
            // Each byte contains two 4-bit codes
            uint8_t code1 = (byte >> 4) & 0x0F;
            uint8_t code2 = byte & 0x0F;
            
            current_value += fib_table[code1];
            if (i * 2 < len) {
                dst[i * 2] = current_value;
            }
            
            current_value += fib_table[code2];
            if (i * 2 + 1 < len) {
                dst[i * 2 + 1] = current_value;
            }
        }
    }
};

struct decoder_8svx::impl {
    // Stream and parsing state
    io_stream* m_stream = nullptr;
    bool m_is_open = false;
    
    // Voice header (VHDR) information
    struct VoiceHeader {
        uint32_t one_shot_hi_samples;   // # samples in one-shot part
        uint32_t repeat_hi_samples;     // # samples in repeat part
        uint32_t samples_per_hi_cycle;  // # samples/cycle in high octave
        uint16_t samples_per_sec;       // Playback rate
        uint8_t octave_count;           // # octaves of waveforms
        uint8_t compression;            // Compression type
        uint32_t volume;                // Fixed-point volume (16.16)
    } m_vhdr = {};
    
    // Sound data
    std::vector<int8_t> m_body_data;
    size_t m_body_size = 0;
    
    // Envelope data
    bool m_has_attack = false;
    bool m_has_release = false;
    std::vector<uint8_t> m_attack_data;
    std::vector<uint8_t> m_release_data;
    
    // Decoding state
    size_t m_current_sample = 0;
    FibonacciDeltaDecoder m_fib_decoder;
    std::vector<int8_t> m_decoded_buffer;
    
    // Parse 8SVX file using libiff
    void parse_file() {
        if (!m_stream) {
            THROW_RUNTIME("No stream to parse");
        }
        
        // Reset to beginning
        m_stream->seek(0, seek_origin::set);
        
        // Set up handler registry
        iff::handler_registry handlers;
        
        // Handle VHDR chunk (required)
        handlers.on_chunk_in_form(
            ESVX_ID,
            VHDR_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_vhdr_chunk(event);
                }
            }
        );
        
        // Handle BODY chunk (required)
        handlers.on_chunk_in_form(
            ESVX_ID,
            BODY_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_body_chunk(event);
                }
            }
        );
        
        // Handle ATAK chunk (optional)
        handlers.on_chunk_in_form(
            ESVX_ID,
            ATAK_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_atak_chunk(event);
                }
            }
        );
        
        // Handle RLSE chunk (optional)
        handlers.on_chunk_in_form(
            ESVX_ID,
            RLSE_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_rlse_chunk(event);
                }
            }
        );
        
        // Create stream adapter for libiff
        StreamAdapter adapter(m_stream);
        
        try {
            iff::parse(adapter, handlers);
        } catch (const std::exception& e) {
            THROW_RUNTIME("Failed to parse 8SVX file: ", e.what());
        }
        
        // Validate we have the required chunks
        if (m_vhdr.samples_per_sec == 0) {
            THROW_RUNTIME("Missing or invalid VHDR chunk");
        }
        if (m_body_size == 0) {
            THROW_RUNTIME("Missing or invalid BODY chunk");
        }
        
        // Handle decompression if needed
        if (m_vhdr.compression == COMP_FIB_DELTA) {
            decompress_fibonacci_delta();
        }
    }
    
    void handle_vhdr_chunk(const iff::chunk_event& event) {
        // Read VHDR data (all fields are big-endian)
        event.reader->read(&m_vhdr.one_shot_hi_samples, 4);
        event.reader->read(&m_vhdr.repeat_hi_samples, 4);
        event.reader->read(&m_vhdr.samples_per_hi_cycle, 4);
        event.reader->read(&m_vhdr.samples_per_sec, 2);
        event.reader->read(&m_vhdr.octave_count, 1);
        event.reader->read(&m_vhdr.compression, 1);
        event.reader->read(&m_vhdr.volume, 4);
        
        // Convert from big-endian
        m_vhdr.one_shot_hi_samples = swap_endian(m_vhdr.one_shot_hi_samples);
        m_vhdr.repeat_hi_samples = swap_endian(m_vhdr.repeat_hi_samples);
        m_vhdr.samples_per_hi_cycle = swap_endian(m_vhdr.samples_per_hi_cycle);
        m_vhdr.samples_per_sec = swap_endian(m_vhdr.samples_per_sec);
        m_vhdr.volume = swap_endian(m_vhdr.volume);
        
        // Validate compression type
        if (m_vhdr.compression > COMP_FIB_DELTA) {
            THROW_RUNTIME("Unsupported 8SVX compression type: ", (int)m_vhdr.compression);
        }
    }
    
    void handle_body_chunk(const iff::chunk_event& event) {
        m_body_size = event.header.size;
        m_body_data.resize(m_body_size);
        
        size_t bytes_read = event.reader->read(m_body_data.data(), m_body_size);
        if (bytes_read != m_body_size) {
            THROW_RUNTIME("Failed to read complete BODY chunk");
        }
    }
    
    void handle_atak_chunk(const iff::chunk_event& event) {
        m_has_attack = true;
        m_attack_data.resize(event.header.size);
        event.reader->read(m_attack_data.data(), event.header.size);
    }
    
    void handle_rlse_chunk(const iff::chunk_event& event) {
        m_has_release = true;
        m_release_data.resize(event.header.size);
        event.reader->read(m_release_data.data(), event.header.size);
    }
    
    void decompress_fibonacci_delta() {
        if (m_vhdr.compression != COMP_FIB_DELTA) {
            return;
        }
        
        // Fibonacci-delta expands 2:1
        size_t decompressed_size = m_body_size * 2;
        m_decoded_buffer.resize(decompressed_size);
        
        m_fib_decoder.reset();
        m_fib_decoder.decode(
            reinterpret_cast<uint8_t*>(m_body_data.data()),
            m_decoded_buffer.data(),
            decompressed_size
        );
        
        // Replace body data with decompressed data
        m_body_data = std::move(m_decoded_buffer);
        m_body_size = decompressed_size;
        m_vhdr.compression = COMP_NONE;  // Mark as decompressed
    }
    
    // Calculate total samples based on octaves
    size_t get_total_samples() const {
        if (m_vhdr.octave_count == 0) {
            return m_body_size;
        }
        
        // For multi-octave samples, calculate total size
        size_t total = 0;
        size_t octave_size = m_vhdr.one_shot_hi_samples + m_vhdr.repeat_hi_samples;
        
        for (uint8_t oct = 0; oct < m_vhdr.octave_count; ++oct) {
            total += octave_size;
            octave_size *= 2;  // Each lower octave is twice as long
        }
        
        return total;
    }
    
    // Stream adapter for libiff (same as AIFF)
    class StreamAdapter : public std::istream {
        struct StreamBuf : public std::streambuf {
            io_stream* m_stream;
            
            StreamBuf(io_stream* stream) : m_stream(stream) {}
            
            int_type underflow() override {
                char ch;
                if (m_stream->read(&ch, 1) == 1) {
                    setg(&ch, &ch, &ch + 1);
                    return traits_type::to_int_type(ch);
                }
                return traits_type::eof();
            }
            
            pos_type seekoff(off_type off, std::ios_base::seekdir dir, 
                           std::ios_base::openmode) override {
                seek_origin origin;
                switch (dir) {
                    case std::ios_base::beg: origin = seek_origin::set; break;
                    case std::ios_base::cur: origin = seek_origin::cur; break;
                    case std::ios_base::end: origin = seek_origin::end; break;
                    default: return pos_type(off_type(-1));
                }
                return m_stream->seek(off, origin);
            }
            
            pos_type seekpos(pos_type pos, std::ios_base::openmode) override {
                return m_stream->seek(pos, seek_origin::set);
            }
        };
        
        StreamBuf m_buf;
        
    public:
        StreamAdapter(io_stream* stream) : std::istream(&m_buf), m_buf(stream) {}
    };
};

// Public interface implementation

decoder_8svx::decoder_8svx() : m_pimpl(std::make_unique<impl>()) {}

decoder_8svx::~decoder_8svx() = default;

bool decoder_8svx::accept(io_stream* rwops) {
    if (!rwops) return false;
    
    auto pos = rwops->tell();
    if (pos < 0) return false;
    
    // Check for FORM chunk
    char chunk_id[4];
    uint32_t size;
    char type_id[4];
    
    if (rwops->read(chunk_id, 4) != 4) {
        rwops->seek(pos, seek_origin::set);
        return false;
    }
    
    if (iff::fourcc(chunk_id) != FORM_ID) {
        rwops->seek(pos, seek_origin::set);
        return false;
    }
    
    // Skip size
    if (rwops->read(&size, 4) != 4) {
        rwops->seek(pos, seek_origin::set);
        return false;
    }
    
    // Check for 8SVX type
    if (rwops->read(type_id, 4) != 4) {
        rwops->seek(pos, seek_origin::set);
        return false;
    }
    
    bool is_8svx = (iff::fourcc(type_id) == ESVX_ID);
    
    rwops->seek(pos, seek_origin::set);
    return is_8svx;
}

const char* decoder_8svx::get_name() const {
    return "8SVX";
}

void decoder_8svx::open(io_stream* rwops) {
    m_pimpl->m_stream = rwops;
    m_pimpl->parse_file();
    m_pimpl->m_is_open = true;
    set_is_open(true);
}

channels_t decoder_8svx::get_channels() const {
    return 1;  // 8SVX is always mono
}

sample_rate_t decoder_8svx::get_rate() const {
    return m_pimpl->m_vhdr.samples_per_sec;
}

bool decoder_8svx::rewind() {
    m_pimpl->m_current_sample = 0;
    if (m_pimpl->m_vhdr.compression == COMP_FIB_DELTA) {
        m_pimpl->m_fib_decoder.reset();
    }
    return true;
}

std::chrono::microseconds decoder_8svx::duration() const {
    if (!is_open() || m_pimpl->m_vhdr.samples_per_sec == 0) {
        return std::chrono::microseconds(0);
    }
    
    // For one-shot samples, return duration of one-shot portion
    // For instruments, return one-shot + one repeat cycle
    size_t total_samples = m_pimpl->m_vhdr.one_shot_hi_samples;
    if (m_pimpl->m_vhdr.repeat_hi_samples > 0) {
        total_samples += m_pimpl->m_vhdr.samples_per_hi_cycle;
    }
    
    double duration_seconds = static_cast<double>(total_samples) / 
                             static_cast<double>(m_pimpl->m_vhdr.samples_per_sec);
    
    return std::chrono::microseconds(
        static_cast<int64_t>(duration_seconds * 1'000'000)
    );
}

bool decoder_8svx::seek_to_time(std::chrono::microseconds pos) {
    if (!is_open() || m_pimpl->m_vhdr.samples_per_sec == 0) {
        return false;
    }
    
    double seconds = pos.count() / 1'000'000.0;
    size_t target_sample = static_cast<size_t>(
        seconds * static_cast<double>(m_pimpl->m_vhdr.samples_per_sec)
    );
    
    size_t total_samples = m_pimpl->get_total_samples();
    if (target_sample >= total_samples) {
        target_sample = total_samples;
    }
    
    m_pimpl->m_current_sample = target_sample;
    return true;
}

bool decoder_8svx::has_repeat() const {
    return m_pimpl->m_vhdr.repeat_hi_samples > 0;
}

uint8_t decoder_8svx::get_octave_count() const {
    return m_pimpl->m_vhdr.octave_count;
}

bool decoder_8svx::is_compressed() const {
    return m_pimpl->m_vhdr.compression != COMP_NONE;
}

float decoder_8svx::get_volume() const {
    // Convert from 16.16 fixed point to float
    return static_cast<float>(m_pimpl->m_vhdr.volume) / 65536.0f;
}

bool decoder_8svx::has_envelope() const {
    return m_pimpl->m_has_attack || m_pimpl->m_has_release;
}

size_t decoder_8svx::do_decode(float* buf, size_t len, bool& call_again) {
    if (!is_open() || m_pimpl->m_current_sample >= m_pimpl->m_body_size) {
        call_again = false;
        return 0;
    }
    
    // Calculate samples to decode
    size_t samples_remaining = m_pimpl->m_body_size - m_pimpl->m_current_sample;
    size_t samples_to_decode = std::min(len, samples_remaining);
    
    // Convert 8-bit signed to float
    const int8_t* src = m_pimpl->m_body_data.data() + m_pimpl->m_current_sample;
    for (size_t i = 0; i < samples_to_decode; ++i) {
        // Convert from signed 8-bit to float [-1.0, 1.0]
        buf[i] = static_cast<float>(src[i]) / 128.0f;
    }
    
    // Apply volume if not unity
    float volume = get_volume();
    if (volume != 1.0f) {
        for (size_t i = 0; i < samples_to_decode; ++i) {
            buf[i] *= volume;
        }
    }
    
    m_pimpl->m_current_sample += samples_to_decode;
    call_again = (m_pimpl->m_current_sample < m_pimpl->m_body_size);
    
    return samples_to_decode;
}

} // namespace musac