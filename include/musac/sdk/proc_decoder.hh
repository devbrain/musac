//
// Created by igor on 3/18/25.
//

#ifndef  PROC_DECODER_HH
#define  PROC_DECODER_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/export_musac_sdk.h>
namespace musac {
    class MUSAC_SDK_EXPORT proc_decoder : public decoder {
        public:
        using loader_fn_t = SDL_AudioSpec* (*)(SDL_IOStream* src, bool closeio, SDL_AudioSpec* spec,
                                               Uint8** audio_buf, Uint32* audio_len);
        public:
            ~proc_decoder() override;
            auto open(SDL_IOStream* rwops) -> bool override;
            [[nodiscard]] unsigned int get_channels() const override;
            [[nodiscard]] unsigned int get_rate() const override;
            bool rewind() override;
            [[nodiscard]] std::chrono::microseconds duration() const override;
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
        unsigned int do_decode(float* buf, unsigned int len, bool& call_again) override;


        protected:
            proc_decoder(loader_fn_t loader_fn);
        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}

#endif
