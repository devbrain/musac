//
// Created by igor on 3/20/25.
//

#ifndef  DECODER_CMF_HH
#define  DECODER_CMF_HH


#include <memory>
#include <musac/sdk/decoder.hh>

namespace musac {
    class decoder_cmf : public decoder {
        public:
            decoder_cmf();
            ~decoder_cmf() override;
            bool open(SDL_IOStream* rwops) override;
            [[nodiscard]] unsigned int get_channels() const override;
            [[nodiscard]] unsigned int get_rate() const override;
            bool rewind() override;
            [[nodiscard]] std::chrono::microseconds duration() const override;
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
            unsigned int do_decode(float buf[], unsigned int len, bool& call_again) override;
        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}


#endif
