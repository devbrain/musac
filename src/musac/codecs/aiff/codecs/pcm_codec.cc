#include <musac/codecs/aiff/aiff_codec_base.hh>
#include <musac/error.hh>
#include <iff/fourcc.hh>
#include <iff/endian.hh>
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace musac {
namespace aiff {

// Compression types
static const iff::fourcc COMP_NONE("NONE");
static const iff::fourcc COMP_SOWT("sowt");  // Little-endian PCM

class pcm_codec : public aiff_codec_base {
public:
    pcm_codec() : m_is_sowt(false) {}
    
    bool accepts(const iff::fourcc& compression_type) const override {
        return compression_type == COMP_NONE || 
               compression_type == COMP_SOWT;
    }
    
    const char* name() const override {
        return m_is_sowt ? "PCM (sowt/little-endian)" : "PCM";
    }
    
    void initialize(const codec_params& params) override {
        m_params = params;
        // Check if this is sowt (little-endian) format
        m_is_sowt = (params.compression_type == COMP_SOWT);
    }
    
    size_t decode(const uint8_t* input, size_t input_bytes, 
                  float* output, size_t output_samples) override {
        size_t samples_available;
        
        if (m_params.bits_per_sample == 12) {
            // 12-bit samples in AIFF are stored in 16-bit containers (not packed)
            samples_available = input_bytes / 2;
        } else {
            size_t bytes_per_sample = (m_params.bits_per_sample + 7) / 8;
            samples_available = input_bytes / bytes_per_sample;
        }
        
        size_t samples_to_decode = std::min(samples_available, output_samples);
        
        switch (m_params.bits_per_sample) {
            case 8:
                return decode_8bit(input, output, samples_to_decode);
            case 16:
                return m_is_sowt ? decode_16bit_le(input, output, samples_to_decode)
                                 : decode_16bit_be(input, output, samples_to_decode);
            case 24:
                return decode_24bit(input, output, samples_to_decode);
            case 32:
                return decode_32bit(input, output, samples_to_decode);
            case 12:
                // 12-bit samples are stored in 16-bit containers
                return decode_12bit_in_16bit(input, output, samples_to_decode);
            default:
                throw std::runtime_error("Unsupported PCM bit depth: " + std::to_string(m_params.bits_per_sample));
        }
    }
    
    size_t get_input_bytes_for_samples(size_t samples) const override {
        if (m_params.bits_per_sample == 12) {
            // 12-bit samples in AIFF are stored in 16-bit containers
            return samples * 2;
        }
        return samples * ((m_params.bits_per_sample + 7) / 8);
    }
    
    size_t get_samples_from_bytes(size_t bytes) const override {
        if (m_params.bits_per_sample == 12) {
            // 12-bit samples in AIFF: 1 sample per 2 bytes
            return bytes / 2;
        }
        return bytes / ((m_params.bits_per_sample + 7) / 8);
    }
    
    void reset() override {
        // PCM is stateless
    }
    
private:
    size_t decode_8bit(const uint8_t* input, float* output, size_t samples) {
        for (size_t i = 0; i < samples; ++i) {
            // 8-bit PCM is signed
            int8_t sample = static_cast<int8_t>(input[i]);
            output[i] = sample / 128.0f;
        }
        return samples;
    }
    
    size_t decode_16bit_be(const uint8_t* input, float* output, size_t samples) {
        const uint16_t* input16 = reinterpret_cast<const uint16_t*>(input);
        for (size_t i = 0; i < samples; ++i) {
            int16_t sample = static_cast<int16_t>(iff::swap16be(input16[i]));
            output[i] = sample / 32768.0f;
        }
        return samples;
    }
    
    size_t decode_16bit_le(const uint8_t* input, float* output, size_t samples) {
        // sowt format - data is already little-endian
        const int16_t* input16 = reinterpret_cast<const int16_t*>(input);
        for (size_t i = 0; i < samples; ++i) {
            output[i] = input16[i] / 32768.0f;
        }
        return samples;
    }
    
    size_t decode_24bit(const uint8_t* input, float* output, size_t samples) {
        for (size_t i = 0; i < samples; ++i) {
            // 24-bit big-endian to signed 32-bit
            int32_t sample = (input[i*3] << 24) | 
                           (input[i*3 + 1] << 16) | 
                           (input[i*3 + 2] << 8);
            sample >>= 8;  // Sign extend
            output[i] = sample / 8388608.0f;
        }
        return samples;
    }
    
    size_t decode_32bit(const uint8_t* input, float* output, size_t samples) {
        const uint32_t* input32 = reinterpret_cast<const uint32_t*>(input);
        for (size_t i = 0; i < samples; ++i) {
            int32_t sample = static_cast<int32_t>(iff::swap32be(input32[i]));
            output[i] = sample / 2147483648.0f;
        }
        return samples;
    }
    
    size_t decode_12bit_in_16bit(const uint8_t* input, float* output, size_t samples) {
        // 12-bit samples in AIFF are stored in 16-bit big-endian containers
        // Despite being labeled as 12-bit, the actual data appears to use the full
        // 16-bit range based on comparison with ffmpeg output.
        // This might be because the 12-bit samples were scaled up to use the full
        // 16-bit range for better precision.
        const uint16_t* input16 = reinterpret_cast<const uint16_t*>(input);
        
        for (size_t i = 0; i < samples; ++i) {
            // Get 16-bit big-endian value and treat as regular 16-bit PCM
            int16_t sample = static_cast<int16_t>(iff::swap16be(input16[i]));
            
            // Convert to float using 16-bit range to match ffmpeg output
            output[i] = sample / 32768.0f;
        }
        
        return samples;
    }
    
private:
    bool m_is_sowt;
};

// Factory function
std::unique_ptr<aiff_codec_base> create_pcm_codec() {
    return std::make_unique<pcm_codec>();
}

} // namespace aiff
} // namespace musac