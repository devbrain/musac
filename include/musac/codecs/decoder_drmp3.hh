// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <musac/sdk/decoder.hh>
#include <musac/codecs/export_musac_codecs.h>
namespace musac {
    /*!
     * \brief dr_mp3 decoder.
     */
    class MUSAC_CODECS_EXPORT decoder_drmp3 : public decoder {
        public:
            decoder_drmp3();
            ~decoder_drmp3() override;

            void open(io_stream* rwops) override;
            [[nodiscard]] unsigned int get_channels() const override;
            [[nodiscard]] unsigned int get_rate() const override;
            bool rewind() override;
            [[nodiscard]] std::chrono::microseconds duration() const override;
            auto seek_to_time(std::chrono::microseconds pos) -> bool override;

        protected:
            unsigned int do_decode(float buf[], unsigned int len, bool& callAgain) override;

        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
