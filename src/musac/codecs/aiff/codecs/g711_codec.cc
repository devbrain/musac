#include <musac/codecs/aiff/aiff_codec_base.hh>
#include <musac/codecs/common/g711_codec.hh>
#include <musac/error.hh>
#include <iff/fourcc.hh>
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace musac::aiff {
    // G.711 compression types (both uppercase and lowercase variants)
    static const iff::fourcc COMP_ULAW("ULAW");
    static const iff::fourcc COMP_ulaw("ulaw");
    static const iff::fourcc COMP_ALAW("ALAW");
    static const iff::fourcc COMP_alaw("alaw");

    class g711_codec : public aiff_codec_base {
        public:
            enum Type {
                ULAW,
                ALAW
            };

            g711_codec()
                : m_type(ULAW) {
            }

            [[nodiscard]] bool accepts(const iff::fourcc& compression_type) const override {
                return compression_type == COMP_ULAW ||
                       compression_type == COMP_ulaw ||
                       compression_type == COMP_ALAW ||
                       compression_type == COMP_alaw;
            }

            [[nodiscard]] const char* name() const override {
                return m_type == ULAW ? "G.711 µ-law" : "G.711 A-law";
            }

            void initialize(const codec_params& params) override {
                m_params = params;

                // G.711 is always 8-bit compressed, but AIFF may report 16-bit in sample_size
                // The v2 decoder doesn't check this, so we'll just accept any bit depth
                // and decode as 8-bit G.711

                // Determine type from compression type
                if (params.compression_type == COMP_ULAW || params.compression_type == COMP_ulaw) {
                    m_type = ULAW;
                } else if (params.compression_type == COMP_ALAW || params.compression_type == COMP_alaw) {
                    m_type = ALAW;
                }
            }

            size_t decode(const uint8_t* input, size_t input_bytes,
                          float* output, size_t output_samples) override {
                size_t samples_to_decode = std::min(input_bytes, output_samples);

                if (m_type == ULAW) {
                    return decode_ulaw(input, output, samples_to_decode);
                } else {
                    return decode_alaw(input, output, samples_to_decode);
                }
            }

            [[nodiscard]] size_t get_input_bytes_for_samples(size_t samples) const override {
                // G.711 is 1 byte per sample
                return samples;
            }

            [[nodiscard]] size_t get_samples_from_bytes(size_t bytes) const override {
                // G.711 is 1 byte per sample
                return bytes;
            }

            void reset() override {
                // G.711 is stateless
            }

        private:
            static size_t decode_ulaw(const uint8_t* input, float* output, size_t samples) {
                // Use common G.711 µ-law decoder
                return codecs::common::decode_ulaw(input, output, samples);
            }

            static size_t decode_alaw(const uint8_t* input, float* output, size_t samples) {
                // Use common G.711 A-law decoder
                return codecs::common::decode_alaw(input, output, samples);
            }

        private:
            Type m_type;
    };

    // Factory functions
    std::unique_ptr <aiff_codec_base> create_ulaw_codec() {
        return std::make_unique <g711_codec>();
    }

    std::unique_ptr <aiff_codec_base> create_alaw_codec() {
        return std::make_unique <g711_codec>();
    }
} // namespace musac::aiff
