// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>
namespace musac {
    /*!
     * \brief dr_mp3 decoder.
     */
    class MUSAC_CODECS_EXPORT decoder_drmp3 : public decoder {
        public:
            decoder_drmp3();
            ~decoder_drmp3() override;

            [[nodiscard]] const char* get_name() const override;
            
            void open(io_stream* rwops) override;
            [[nodiscard]] channels_t get_channels() const override;
            [[nodiscard]] sample_rate_t get_rate() const override;
            bool rewind() override;
            [[nodiscard]] std::chrono::microseconds duration() const override;
            auto seek_to_time(std::chrono::microseconds pos) -> bool override;

        protected:
            [[nodiscard]] bool do_accept(io_stream* rwops) override;
            size_t do_decode(float buf[], size_t len, bool& callAgain) override;

        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
