#include <musac/codecs/common/adpcm_codec.hh>
#include <algorithm>

namespace musac::codecs::common {

// Creative ADPCM step table for 4-bit
const creative_adpcm_codec::step_table_t creative_adpcm_codec::step_table = {{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
}};

// Index adjustments for Creative ADPCM 4-bit
const creative_adpcm_codec::index_table_t creative_adpcm_codec::index_table = {{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
}};

// Helper function for 4-bit ADPCM sample decoding
static int16_t decode_adpcm_4bit_sample(uint8_t nibble, creative_adpcm_codec::state& state) {
    int16_t step = creative_adpcm_codec::step_table[state.step_idx];
    int16_t diff = step >> 3;
    
    if (nibble & 4) diff += step;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 1) diff += step >> 2;
    if (nibble & 8) diff = -diff;
    
    state.sample = std::clamp<int16_t>(state.sample + diff, -32768, 32767);
    
    state.step_idx += creative_adpcm_codec::index_table[nibble];
    state.step_idx = std::clamp<uint8_t>(state.step_idx, 0, 88);
    
    return state.sample;
}

size_t creative_adpcm_codec::decode_4bit(const uint8_t* input, size_t input_bytes,
                                         float* output, state& decoder_state) {
    size_t output_samples = 0;
    
    // Each byte contains two 4-bit samples
    for (size_t i = 0; i < input_bytes; ++i) {
        uint8_t byte = input[i];
        
        // Low nibble first
        int16_t sample1 = decode_adpcm_4bit_sample(byte & 0x0F, decoder_state);
        output[output_samples++] = static_cast<float>(sample1) / 32768.0f;
        
        // High nibble second
        int16_t sample2 = decode_adpcm_4bit_sample((byte >> 4) & 0x0F, decoder_state);
        output[output_samples++] = static_cast<float>(sample2) / 32768.0f;
    }
    
    return output_samples;
}

size_t creative_adpcm_codec::decode_4bit_to_int16(const uint8_t* input, size_t input_bytes,
                                                  int16_t* output, state& decoder_state) {
    size_t output_samples = 0;
    
    // For Creative VOC ADPCM, the first byte contains the initial predictor
    // stored as a nibble-packed value that should be decoded normally
    // (No special handling needed - process all bytes as ADPCM data)
    
    // Each byte contains two 4-bit samples
    for (size_t i = 0; i < input_bytes; ++i) {
        uint8_t byte = input[i];
        
        // Low nibble first
        output[output_samples++] = decode_adpcm_4bit_sample(byte & 0x0F, decoder_state);
        
        // High nibble second
        output[output_samples++] = decode_adpcm_4bit_sample((byte >> 4) & 0x0F, decoder_state);
    }
    
    return output_samples;
}

// Creative ADPCM 2.6-bit decoder
// This format packs 3 samples into 1 byte (8 bits / 3 ≈ 2.67 bits per sample)
size_t creative_adpcm_codec::decode_26bit(const uint8_t* input, size_t input_bytes,
                                          float* output, state& decoder_state) {
    size_t output_samples = 0;
    
    // Creative 2.6-bit ADPCM uses delta coding without a reference byte
    // All bytes contain packed ADPCM data
    
    // Each byte contains 3 samples encoded as deltas
    static const int16_t delta_table_26[8] = {
        -3, -2, -1, 0, 1, 2, 3, 4
    };
    
    for (size_t i = 0; i < input_bytes; ++i) {
        uint8_t byte = input[i];
        
        // Extract three 2.67-bit values from byte
        uint8_t delta1 = (byte >> 5) & 0x07;
        uint8_t delta2 = (byte >> 2) & 0x07;
        uint8_t delta3 = byte & 0x03;
        
        // Apply deltas
        decoder_state.sample = std::clamp<int16_t>(
            decoder_state.sample + delta_table_26[delta1] * 256, -32768, 32767);
        output[output_samples++] = static_cast<float>(decoder_state.sample) / 32768.0f;
        
        decoder_state.sample = std::clamp<int16_t>(
            decoder_state.sample + delta_table_26[delta2] * 256, -32768, 32767);
        output[output_samples++] = static_cast<float>(decoder_state.sample) / 32768.0f;
        
        // Third sample only uses 2 bits
        decoder_state.sample = std::clamp<int16_t>(
            decoder_state.sample + (delta3 - 1) * 256, -32768, 32767);
        output[output_samples++] = static_cast<float>(decoder_state.sample) / 32768.0f;
    }
    
    return output_samples;
}

size_t creative_adpcm_codec::decode_26bit_to_int16(const uint8_t* input, size_t input_bytes,
                                                   int16_t* output, state& decoder_state) {
    size_t output_samples = 0;
    
    // No reference byte - all bytes contain packed ADPCM data
    
    static const int16_t delta_table_26[8] = {
        -3, -2, -1, 0, 1, 2, 3, 4
    };
    
    for (size_t i = 0; i < input_bytes; ++i) {
        uint8_t byte = input[i];
        
        uint8_t delta1 = (byte >> 5) & 0x07;
        uint8_t delta2 = (byte >> 2) & 0x07;
        uint8_t delta3 = byte & 0x03;
        
        decoder_state.sample = std::clamp<int16_t>(
            decoder_state.sample + delta_table_26[delta1] * 256, -32768, 32767);
        output[output_samples++] = decoder_state.sample;
        
        decoder_state.sample = std::clamp<int16_t>(
            decoder_state.sample + delta_table_26[delta2] * 256, -32768, 32767);
        output[output_samples++] = decoder_state.sample;
        
        decoder_state.sample = std::clamp<int16_t>(
            decoder_state.sample + (delta3 - 1) * 256, -32768, 32767);
        output[output_samples++] = decoder_state.sample;
    }
    
    return output_samples;
}

// Creative ADPCM 2-bit decoder
// This format packs 4 samples into 1 byte
size_t creative_adpcm_codec::decode_2bit(const uint8_t* input, size_t input_bytes,
                                         float* output, state& decoder_state) {
    size_t output_samples = 0;
    
    // No reference byte - all bytes contain packed ADPCM data
    
    // Each byte contains 4 samples encoded as 2-bit deltas
    static const int16_t delta_table_2[4] = {
        -2, -1, 1, 2
    };
    
    for (size_t i = 0; i < input_bytes; ++i) {
        uint8_t byte = input[i];
        
        // Extract four 2-bit values from byte
        for (int j = 0; j < 4; ++j) {
            uint8_t delta_idx = (byte >> (j * 2)) & 0x03;
            decoder_state.sample = std::clamp<int16_t>(
                decoder_state.sample + delta_table_2[delta_idx] * 256, -32768, 32767);
            output[output_samples++] = static_cast<float>(decoder_state.sample) / 32768.0f;
        }
    }
    
    return output_samples;
}

size_t creative_adpcm_codec::decode_2bit_to_int16(const uint8_t* input, size_t input_bytes,
                                                  int16_t* output, state& decoder_state) {
    size_t output_samples = 0;
    
    // No reference byte - all bytes contain packed ADPCM data
    
    static const int16_t delta_table_2[4] = {
        -2, -1, 1, 2
    };
    
    for (size_t i = 0; i < input_bytes; ++i) {
        uint8_t byte = input[i];
        
        for (int j = 0; j < 4; ++j) {
            uint8_t delta_idx = (byte >> (j * 2)) & 0x03;
            decoder_state.sample = std::clamp<int16_t>(
                decoder_state.sample + delta_table_2[delta_idx] * 256, -32768, 32767);
            output[output_samples++] = decoder_state.sample;
        }
    }
    
    return output_samples;
}

} // namespace musac::codecs::common