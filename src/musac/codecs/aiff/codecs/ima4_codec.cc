#include <musac/codecs/aiff/aiff_codec_base.hh>
#include <musac/error.hh>
#include <iff/fourcc.hh>
#include <iff/endian.hh>
#include <algorithm>
#include <vector>
#include <cstring>
#include <stdexcept>

namespace musac {
namespace aiff {

// IMA4 compression type
static const iff::fourcc COMP_IMA4("ima4");

// IMA ADPCM tables
static const int16_t ima_step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const int8_t ima_index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

// IMA4 ADPCM decoder implementation (from decoder_aiff_v2)
class IMA4Decoder {
    int32_t predictor = 0;
    int8_t step_index = 0;
    
    int16_t decode_sample(uint8_t nibble) {
        // Ensure step_index is in valid range to avoid undefined behavior
        // This can happen with malformed files
        if (step_index < 0) step_index = 0;
        if (step_index > 88) {
            step_index = 88;
        }
        
        int step = ima_step_table[step_index];
        int diff = step >> 3;
        
        if (nibble & 4) diff += step;
        if (nibble & 2) diff += step >> 1;
        if (nibble & 1) diff += step >> 2;
        
        if (nibble & 8) {
            predictor -= diff;
        } else {
            predictor += diff;
        }
        
        // Clamp predictor to 16-bit range
        if (predictor > 32767) predictor = 32767;
        else if (predictor < -32768) predictor = -32768;
        
        // Update step index
        step_index += ima_index_table[nibble];
        if (step_index < 0) step_index = 0;
        else if (step_index > 88) step_index = 88;
        
        return static_cast<int16_t>(predictor);
    }
    
public:
    void reset(int16_t initial_predictor, int8_t initial_index) {
        predictor = initial_predictor;
        step_index = initial_index;
    }
    
    void decode_block(const uint8_t* src, int16_t* dst, size_t channels) {
        // IMA4 block format per channel:
        // 2 bytes: initial predictor (big-endian)
        // 1 byte: initial step index
        // 31 bytes: compressed samples (62 nibbles = 62 samples)
        // Plus 1 initial sample from predictor = 64 samples total
        
        for (size_t ch = 0; ch < channels; ++ch) {
            const uint8_t* ch_src = src + ch * 34;
            int16_t* ch_dst = dst + ch * 64;
            
            // Read header (2-byte preamble)
            // The preamble contains both predictor and step index packed together
            uint16_t preamble = iff::swap16be(*reinterpret_cast<const uint16_t*>(ch_src));
            int16_t initial_predictor = static_cast<int16_t>(preamble & 0xFF80);  // Upper 9 bits
            int8_t initial_index = preamble & 0x7F;  // Lower 7 bits
            
            // Reset decoder state
            reset(initial_predictor, initial_index);
            
            // First sample is the initial predictor
            ch_dst[0] = initial_predictor;
            
            // Decode 63 samples (stored as nibbles)
            // Header is 2 bytes, so data starts at offset 2
            const uint8_t* data = ch_src + 2;
            
            // Apple QuickTime IMA4: nibbles are decoded bottom-to-top (low nibble first)
            // This is opposite of standard IMA ADPCM!
            // 32 bytes contain 64 nibbles, but we only use 63 (plus the initial predictor = 64 total)
            for (int i = 0; i < 31; ++i) {
                uint8_t byte = data[i];
                // Low nibble first, then high nibble
                ch_dst[i * 2 + 1] = decode_sample(byte & 0x0F);
                ch_dst[i * 2 + 2] = decode_sample((byte >> 4) & 0x0F);
            }
            // Last byte (byte 31) has only one nibble used (the low nibble)
            ch_dst[63] = decode_sample(data[31] & 0x0F);
        }
    }
};

class ima4_codec : public aiff_codec_base {
public:
    ima4_codec() = default;
    
    bool accepts(const iff::fourcc& compression_type) const override {
        return compression_type == COMP_IMA4;
    }
    
    const char* name() const override {
        return "IMA4 ADPCM";
    }
    
    void initialize(const codec_params& params) override {
        m_params = params;
        
        // IMA4 specific validation
        if (params.frames_per_packet != 64) {
            throw std::runtime_error("IMA4 requires 64 frames per packet");
        }
        if (params.bytes_per_packet != 34) {
            throw std::runtime_error("IMA4 requires 34 bytes per packet");
        }
    }
    
    size_t decode(const uint8_t* input, size_t input_bytes, 
                  float* output, size_t output_samples) override {
        // IMA4 decodes in blocks of 64 samples per channel
        // Each block is 34 bytes per channel
        
        size_t channels = m_params.channels;
        size_t block_size = 34 * channels;
        size_t samples_per_block = 64 * channels;
        
        // Check how many complete blocks we have
        size_t complete_blocks = input_bytes / block_size;
        size_t samples_available = complete_blocks * samples_per_block;
        size_t samples_to_decode = std::min(samples_available, output_samples);
        
        // Round down to complete blocks
        size_t blocks_to_decode = samples_to_decode / samples_per_block;
        samples_to_decode = blocks_to_decode * samples_per_block;
        
        if (samples_to_decode == 0) {
            return 0;
        }
        
        // Temporary PCM buffer for one block
        std::vector<int16_t> pcm_buffer(64 * channels);
        
        // Create fresh decoder for each decode call to match V2 behavior
        // V2 creates new decoder in convert_ima4_to_float each time
        IMA4Decoder fresh_decoder;
        
        size_t input_offset = 0;
        size_t output_offset = 0;
        
        for (size_t block = 0; block < blocks_to_decode; ++block) {
            // Decode one block using fresh decoder
            fresh_decoder.decode_block(input + input_offset, pcm_buffer.data(), channels);
            
            // Convert to float and interleave channels
            for (size_t frame = 0; frame < 64; ++frame) {
                for (size_t ch = 0; ch < channels; ++ch) {
                    output[output_offset++] = pcm_buffer[ch * 64 + frame] / 32768.0f;
                }
            }
            
            input_offset += block_size;
        }
        
        return samples_to_decode;
    }
    
    size_t get_input_bytes_for_samples(size_t samples) const override {
        // IMA4: 34 bytes per 64 samples per channel
        size_t frames = samples / m_params.channels;
        size_t blocks = (frames + 63) / 64;  // Round up to complete blocks
        return blocks * 34 * m_params.channels;
    }
    
    size_t get_samples_from_bytes(size_t bytes) const override {
        // IMA4: 34 bytes per 64 samples per channel
        size_t blocks = bytes / (34 * m_params.channels);
        return blocks * 64 * m_params.channels;
    }
    
    void reset() override {
        // No persistent state to reset since we create fresh decoder each time
    }
    
    size_t get_block_align() const override {
        // IMA4 must be decoded in complete blocks
        return 64;  // 64 samples per block
    }
    
private:
    // No persistent decoder - create fresh one each decode() call to match V2
};

// Factory function
std::unique_ptr<aiff_codec_base> create_ima4_codec() {
    return std::make_unique<ima4_codec>();
}

} // namespace aiff
} // namespace musac