//
// Created by igor on 3/19/25.
//

#ifndef  DECODER_OPB_HH
#define  DECODER_OPB_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    class MUSAC_CODECS_EXPORT decoder_opb : public decoder {
        public:
            decoder_opb();
            ~decoder_opb() override;
            auto open(io_stream* rwops) -> bool override;
            [[nodiscard]] unsigned int get_channels() const override;
            [[nodiscard]] unsigned int get_rate() const override;
            bool rewind() override;
            [[nodiscard]] std::chrono::microseconds duration() const override;
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
            unsigned int do_decode(float* buf, unsigned int len, bool& callAgain) override;

        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}

#endif
