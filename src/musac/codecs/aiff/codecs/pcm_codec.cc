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
            // 12-bit samples are packed: 2 samples in 3 bytes
            samples_available = (input_bytes * 2) / 3;
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
                // Pass input_bytes directly since decode_12bit_packed needs it for bounds checking
                return decode_12bit_packed(input, input_bytes, output, samples_to_decode);
            default:
                throw std::runtime_error("Unsupported PCM bit depth: " + std::to_string(m_params.bits_per_sample));
        }
    }
    
    size_t get_input_bytes_for_samples(size_t samples) const override {
        if (m_params.bits_per_sample == 12) {
            // 12-bit samples are packed: 2 samples in 3 bytes
            return (samples * 3 + 1) / 2;
        }
        return samples * ((m_params.bits_per_sample + 7) / 8);
    }
    
    size_t get_samples_from_bytes(size_t bytes) const override {
        if (m_params.bits_per_sample == 12) {
            // 12-bit samples: 2 samples per 3 bytes
            return (bytes * 2) / 3;
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
    
    size_t decode_12bit_packed(const uint8_t* input, size_t input_bytes, float* output, size_t samples) {
        // 12-bit samples are packed: 2 samples in 3 bytes
        // Format (big-endian):
        // First sample: byte0 + high nibble of byte1
        // Second sample: low nibble of byte1 + byte2
        
        size_t output_idx = 0;
        size_t input_idx = 0;
        
        while (output_idx < samples && input_idx + 2 <= input_bytes) {
            // First sample: byte0 + high nibble of byte1
            int32_t sample1 = (static_cast<int32_t>(input[input_idx]) << 24) |
                             (static_cast<int32_t>(input[input_idx + 1] & 0xF0) << 16);
            sample1 = sample1 >> 20;  // Shift to get 12-bit value with sign extension
            if (sample1 & 0x800) sample1 |= 0xFFFFF000;  // Sign extend
            output[output_idx++] = static_cast<float>(sample1) / 2048.0f;
            
            if (output_idx >= samples) break;
            
            // Second sample: low nibble of byte1 + byte2
            int32_t sample2 = (static_cast<int32_t>(input[input_idx + 1] & 0x0F) << 28) |
                             (static_cast<int32_t>(input[input_idx + 2]) << 20);
            sample2 = sample2 >> 20;  // Shift to get 12-bit value with sign extension
            if (sample2 & 0x800) sample2 |= 0xFFFFF000;  // Sign extend
            output[output_idx++] = static_cast<float>(sample2) / 2048.0f;
            
            input_idx += 3;
        }
        
        // Handle last sample if there's an odd number and we have at least 2 bytes left
        if (output_idx < samples && input_idx + 1 <= input_bytes) {
            int32_t sample1 = (static_cast<int32_t>(input[input_idx]) << 24) |
                             (static_cast<int32_t>(input[input_idx + 1] & 0xF0) << 16);
            sample1 = sample1 >> 20;  // Shift to get 12-bit value with sign extension
            if (sample1 & 0x800) sample1 |= 0xFFFFF000;  // Sign extend
            output[output_idx++] = static_cast<float>(sample1) / 2048.0f;
        }
        
        return output_idx;
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