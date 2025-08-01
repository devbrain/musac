//
// Created by igor on 3/18/25.
//

#ifndef  DECODER_VOC_HH
#define  DECODER_VOC_HH

#include <memory>
#include <musac/sdk/sdl_compat.h>
#include <musac/sdk/decoder.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    class MUSAC_CODECS_EXPORT decoder_voc : public decoder {
        public:
            decoder_voc();
            ~decoder_voc() override;
            
            [[nodiscard]] bool open(SDL_IOStream* rwops) override;
            [[nodiscard]] unsigned int get_channels() const override;
            [[nodiscard]] unsigned int get_rate() const override;
            bool rewind() override;
            [[nodiscard]] std::chrono::microseconds duration() const override;
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
            unsigned int do_decode(float* buf, unsigned int len, bool& call_again) override;

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}

#endif
