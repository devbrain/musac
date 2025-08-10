//
// Created by igor on 3/18/25.
//

#ifndef  DECODER_AIFF_HH
#define  DECODER_AIFF_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    class MUSAC_CODECS_EXPORT decoder_aiff : public decoder {
        public:
            decoder_aiff();
            ~decoder_aiff() override;
            
            // Static method to check if this decoder can handle the format
            [[nodiscard]] static bool accept(io_stream* rwops);
            
            [[nodiscard]] const char* get_name() const override;
            
            void open(io_stream* rwops) override;
            [[nodiscard]] channels_t get_channels() const override;
            [[nodiscard]] sample_rate_t get_rate() const override;
            bool rewind() override;
            [[nodiscard]] std::chrono::microseconds duration() const override;
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
            size_t do_decode(float* buf, size_t len, bool& call_again) override;

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}

#endif
