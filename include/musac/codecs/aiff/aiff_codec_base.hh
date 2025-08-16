#pragma once

#include <musac/sdk/types.hh>
#include <iff/fourcc.hh>
#include <memory>
#include <cstddef>

namespace musac {
namespace aiff {

// Parameters for codec initialization
struct codec_params {
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
    uint32_t num_frames;
    iff::fourcc compression_type;  // Compression type for codec-specific handling
    
    // Optional parameters for specific codecs
    uint32_t frames_per_packet = 0;  // For IMA4
    uint32_t bytes_per_packet = 0;   // For IMA4
};

// Base class for all AIFF compression codecs
class aiff_codec_base {
public:
    virtual ~aiff_codec_base() = default;
    
    // Check if this codec can handle the given compression type
    virtual bool accepts(const iff::fourcc& compression_type) const = 0;
    
    // Get human-readable name of the codec
    virtual const char* name() const = 0;
    
    // Initialize the codec with format parameters
    virtual void initialize(const codec_params& params) = 0;
    
    // Decode compressed audio data to float samples
    // Returns number of samples actually decoded
    virtual size_t decode(
        const uint8_t* input,
        size_t input_bytes,
        float* output,
        size_t output_samples
    ) = 0;
    
    // Get the number of input bytes needed to produce the given number of output samples
    virtual size_t get_input_bytes_for_samples(size_t samples) const = 0;
    
    // Get block alignment requirements (1 for uncompressed, block size for compressed)
    virtual size_t get_block_align() const { return 1; }
    
    // Reset codec state (for seeking)
    virtual void reset() = 0;
    
    // Get the number of samples that will be produced from the given input size
    virtual size_t get_samples_from_bytes(size_t bytes) const = 0;
    
protected:
    codec_params m_params;
};

} // namespace aiff
} // namespace musac