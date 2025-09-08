#include <musac/codecs/aiff/aiff_codec_base.hh>
#include <musac/error.hh>
#include <iff/fourcc.hh>
#include <iff/endian.hh>
#include <iff/endian.hh>
#include <algorithm>
#include <cstring>

namespace musac {
namespace aiff {

// Float compression types
static const iff::fourcc COMP_FL32("fl32");
static const iff::fourcc COMP_FL64("fl64");

class float_codec : public aiff_codec_base {
public:
    float_codec() : m_is_64bit(false) {}
    
    bool accepts(const iff::fourcc& compression_type) const override {
        return compression_type == COMP_FL32 || 
               compression_type == COMP_FL64;
    }
    
    const char* name() const override {
        return m_is_64bit ? "Float64" : "Float32";
    }
    
    void initialize(const codec_params& params) override {
        m_params = params;
        // Check if this is 64-bit float
        m_is_64bit = (params.compression_type == COMP_FL64);
    }
    
    size_t decode(const uint8_t* input, size_t input_bytes, 
                  float* output, size_t output_samples) override {
        if (m_is_64bit) {
            return decode_float64(input, input_bytes, output, output_samples);
        } else {
            return decode_float32(input, input_bytes, output, output_samples);
        }
    }
    
    size_t get_input_bytes_for_samples(size_t samples) const override {
        return samples * (m_is_64bit ? 8 : 4);
    }
    
    size_t get_samples_from_bytes(size_t bytes) const override {
        return bytes / (m_is_64bit ? 8 : 4);
    }
    
    void reset() override {
        // Float codec is stateless
    }
    
private:
    size_t decode_float32(const uint8_t* input, size_t input_bytes,
                         float* output, size_t output_samples) {
        size_t samples_available = input_bytes / 4;
        size_t samples_to_decode = std::min(samples_available, output_samples);
        
        const uint32_t* input32 = reinterpret_cast<const uint32_t*>(input);
        
        for (size_t i = 0; i < samples_to_decode; ++i) {
            // Read 32-bit big-endian float
            uint32_t bits = iff::swap32be(input32[i]);
            
            // Interpret as float
            float value;
            std::memcpy(&value, &bits, sizeof(float));
            output[i] = value;
        }
        
        return samples_to_decode;
    }
    
    size_t decode_float64(const uint8_t* input, size_t input_bytes,
                         float* output, size_t output_samples) {
        size_t samples_available = input_bytes / 8;
        size_t samples_to_decode = std::min(samples_available, output_samples);
        
        const uint64_t* input64 = reinterpret_cast<const uint64_t*>(input);
        
        for (size_t i = 0; i < samples_to_decode; ++i) {
            // Read 64-bit big-endian double
            uint64_t bits = iff::swap64be(input64[i]);
            
            // Interpret as double
            double value;
            std::memcpy(&value, &bits, sizeof(double));
            
            // Convert to float (with precision loss)
            output[i] = static_cast<float>(value);
        }
        
        return samples_to_decode;
    }
    
private:
    bool m_is_64bit;
};

// Factory function
std::unique_ptr<aiff_codec_base> create_float_codec() {
    return std::make_unique<float_codec>();
}

} // namespace aiff
} // namespace musac