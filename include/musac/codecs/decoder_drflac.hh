// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <musac/sdk/decoder.hh>

namespace musac {
    /*!
     * \brief dr_flac decoder.
     */
    class decoder_drflac : public decoder {
        public:
            decoder_drflac();
            ~decoder_drflac() override;

            bool open(SDL_IOStream* rwops) override;
            [[nodiscard]] unsigned int get_channels() const override;
            [[nodiscard]] unsigned int get_rate() const override;
            bool rewind() override;
            [[nodiscard]] auto duration() const -> std::chrono::microseconds override;
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
            unsigned int do_decode(float* buf, unsigned int len, bool& callAgain) override;

        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
