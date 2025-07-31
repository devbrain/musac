//
// Created by igor on 3/23/25.
//

#ifndef  DECODER_SEQ_HH
#define  DECODER_SEQ_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    class MUSAC_CODECS_EXPORT decoder_seq : public decoder {
        public:
            decoder_seq();
            ~decoder_seq() override;
            bool open(SDL_IOStream* rwops) override;
            [[nodiscard]] unsigned int get_channels() const override;
            [[nodiscard]] unsigned int get_rate() const override;
            bool rewind() override;
            [[nodiscard]] auto duration() const -> std::chrono::microseconds override;
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
            unsigned int do_decode(float buf[], unsigned int len, bool& call_again) override;
        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}



#endif
