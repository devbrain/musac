#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

namespace musac::codecs::common {
    /**
     * @brief G.711 codec class providing A-law and µ-law decoding
     *
     * This class provides static methods for decoding G.711 compressed audio
     * (A-law and µ-law) to either floating-point or 16-bit integer samples.
     *
     * G.711 is an ITU-T standard for audio companding used primarily in telephony.
     */
    class g711_codec {
        public:
            // Lookup tables for G.711 decompression
            static constexpr size_t TABLE_SIZE = 256;
            using lookup_table_t = std::array <int16_t, TABLE_SIZE>;

            // G.711 µ-law decompression table
            static const lookup_table_t ulaw_table;

            // G.711 A-law decompression table
            static const lookup_table_t alaw_table;

            /**
             * @brief Decode µ-law samples to float
             * @param input Pointer to input µ-law samples
             * @param output Pointer to output float buffer
             * @param samples Number of samples to decode
             * @return Number of samples decoded
             */
            [[nodiscard]] static size_t decode_ulaw(const uint8_t* input, float* output, size_t samples);

            /**
             * @brief Decode A-law samples to float
             * @param input Pointer to input A-law samples
             * @param output Pointer to output float buffer
             * @param samples Number of samples to decode
             * @return Number of samples decoded
             */
            [[nodiscard]] static size_t decode_alaw(const uint8_t* input, float* output, size_t samples);

            /**
             * @brief Decode µ-law samples to int16
             * @param input Pointer to input µ-law samples
             * @param output Pointer to output int16 buffer
             * @param samples Number of samples to decode
             * @return Number of samples decoded
             */
            [[nodiscard]] static size_t decode_ulaw_to_int16(const uint8_t* input, int16_t* output, size_t samples);

            /**
             * @brief Decode A-law samples to int16
             * @param input Pointer to input A-law samples
             * @param output Pointer to output int16 buffer
             * @param samples Number of samples to decode
             * @return Number of samples decoded
             */
            [[nodiscard]] static size_t decode_alaw_to_int16(const uint8_t* input, int16_t* output, size_t samples);

            // Deleted constructor to prevent instantiation (all methods are static)
            g711_codec() = delete;
    };

    // Convenience free functions that delegate to the class
    inline size_t decode_ulaw(const uint8_t* input, float* output, size_t samples) {
        return g711_codec::decode_ulaw(input, output, samples);
    }

    inline size_t decode_alaw(const uint8_t* input, float* output, size_t samples) {
        return g711_codec::decode_alaw(input, output, samples);
    }

    inline size_t decode_ulaw_to_int16(const uint8_t* input, int16_t* output, size_t samples) {
        return g711_codec::decode_ulaw_to_int16(input, output, samples);
    }

    inline size_t decode_alaw_to_int16(const uint8_t* input, int16_t* output, size_t samples) {
        return g711_codec::decode_alaw_to_int16(input, output, samples);
    }
} // namespace musac::codecs::common
