#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

namespace musac::codecs::common {
    /**
     * @brief Creative ADPCM codec for VOC files
     *
     * This class provides static methods for decoding Creative ADPCM compressed audio
     * used in VOC (Creative Voice) files. Supports 4-bit, 2.6-bit, and 2-bit variants.
     *
     * ADPCM (Adaptive Differential Pulse Code Modulation) reduces data size by
     * encoding the difference between samples rather than absolute values.
     */
    class creative_adpcm_codec {
        public:
            /**
             * @brief Decoder state for stateful ADPCM decoding
             *
             * ADPCM is a stateful codec that requires maintaining the previous
             * sample value and step index between decode calls.
             */
            struct state {
                int16_t sample = 0; ///< Current sample value
                uint8_t step_idx = 0; ///< Index into step table

                constexpr state() noexcept = default;

                /**
                 * @brief Reset the decoder state to initial values
                 */
                constexpr void reset() noexcept {
                    sample = 0;
                    step_idx = 0;
                }
            };

            /**
             * @brief Bit depths supported by Creative ADPCM
             */
            enum class bit_depth {
                bits_4, ///< 4-bit ADPCM (2 samples per byte)
                bits_26, ///< 2.6-bit ADPCM (3 samples per byte)
                bits_2 ///< 2-bit ADPCM (4 samples per byte)
            };

            // Step table for Creative ADPCM
            static constexpr size_t STEP_TABLE_SIZE = 89;
            using step_table_t = std::array <int16_t, STEP_TABLE_SIZE>;
            static const step_table_t step_table;

            // Index adjustment table for 4-bit ADPCM
            static constexpr size_t INDEX_TABLE_SIZE = 16;
            using index_table_t = std::array <int8_t, INDEX_TABLE_SIZE>;
            static const index_table_t index_table;

            /**
             * @brief Decode 4-bit ADPCM samples to float
             * @param input Pointer to input ADPCM data
             * @param input_bytes Number of input bytes
             * @param output Pointer to output float buffer
             * @param decoder_state Current decoder state (will be updated)
             * @return Number of samples decoded (2 per input byte)
             */
            [[nodiscard]] static size_t decode_4bit(const uint8_t* input, size_t input_bytes,
                                                    float* output, state& decoder_state);

            /**
             * @brief Decode 4-bit ADPCM samples to int16
             * @param input Pointer to input ADPCM data
             * @param input_bytes Number of input bytes
             * @param output Pointer to output int16 buffer
             * @param decoder_state Current decoder state (will be updated)
             * @return Number of samples decoded (2 per input byte)
             */
            [[nodiscard]] static size_t decode_4bit_to_int16(const uint8_t* input, size_t input_bytes,
                                                             int16_t* output, state& decoder_state);

            /**
             * @brief Decode 2.6-bit ADPCM samples to float
             * @param input Pointer to input ADPCM data
             * @param input_bytes Number of input bytes
             * @param output Pointer to output float buffer
             * @param decoder_state Current decoder state (will be updated)
             * @return Number of samples decoded (3 per input byte)
             */
            [[nodiscard]] static size_t decode_26bit(const uint8_t* input, size_t input_bytes,
                                                     float* output, state& decoder_state);

            /**
             * @brief Decode 2.6-bit ADPCM samples to int16
             * @param input Pointer to input ADPCM data
             * @param input_bytes Number of input bytes
             * @param output Pointer to output int16 buffer
             * @param decoder_state Current decoder state (will be updated)
             * @return Number of samples decoded (3 per input byte)
             */
            [[nodiscard]] static size_t decode_26bit_to_int16(const uint8_t* input, size_t input_bytes,
                                                              int16_t* output, state& decoder_state);

            /**
             * @brief Decode 2-bit ADPCM samples to float
             * @param input Pointer to input ADPCM data
             * @param input_bytes Number of input bytes
             * @param output Pointer to output float buffer
             * @param decoder_state Current decoder state (will be updated)
             * @return Number of samples decoded (4 per input byte)
             */
            [[nodiscard]] static size_t decode_2bit(const uint8_t* input, size_t input_bytes,
                                                    float* output, state& decoder_state);

            /**
             * @brief Decode 2-bit ADPCM samples to int16
             * @param input Pointer to input ADPCM data
             * @param input_bytes Number of input bytes
             * @param output Pointer to output int16 buffer
             * @param decoder_state Current decoder state (will be updated)
             * @return Number of samples decoded (4 per input byte)
             */
            [[nodiscard]] static size_t decode_2bit_to_int16(const uint8_t* input, size_t input_bytes,
                                                             int16_t* output, state& decoder_state);

        public:
            // Deleted constructor to prevent instantiation (all methods are static)
            creative_adpcm_codec() = delete;
    };

    // Type alias for backward compatibility
    using creative_adpcm_state = creative_adpcm_codec::state;

    // Free functions for backward compatibility
    inline size_t decode_creative_adpcm_4bit(const uint8_t* input, size_t input_bytes,
                                             float* output, creative_adpcm_state& state) {
        return creative_adpcm_codec::decode_4bit(input, input_bytes, output, state);
    }

    inline size_t decode_creative_adpcm_4bit_to_int16(const uint8_t* input, size_t input_bytes,
                                                      int16_t* output, creative_adpcm_state& state) {
        return creative_adpcm_codec::decode_4bit_to_int16(input, input_bytes, output, state);
    }

    inline size_t decode_creative_adpcm_26bit(const uint8_t* input, size_t input_bytes,
                                              float* output, creative_adpcm_state& state) {
        return creative_adpcm_codec::decode_26bit(input, input_bytes, output, state);
    }

    inline size_t decode_creative_adpcm_26bit_to_int16(const uint8_t* input, size_t input_bytes,
                                                       int16_t* output, creative_adpcm_state& state) {
        return creative_adpcm_codec::decode_26bit_to_int16(input, input_bytes, output, state);
    }

    inline size_t decode_creative_adpcm_2bit(const uint8_t* input, size_t input_bytes,
                                             float* output, creative_adpcm_state& state) {
        return creative_adpcm_codec::decode_2bit(input, input_bytes, output, state);
    }

    inline size_t decode_creative_adpcm_2bit_to_int16(const uint8_t* input, size_t input_bytes,
                                                      int16_t* output, creative_adpcm_state& state) {
        return creative_adpcm_codec::decode_2bit_to_int16(input, input_bytes, output, state);
    }
} // namespace musac::codecs::common
