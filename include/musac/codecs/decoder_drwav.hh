// This is copyrighted software. More information is at the end of this file.

#pragma once

#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>
namespace musac {
    /*!
     * \brief dr_wav decoder.
     */
    class MUSAC_CODECS_EXPORT decoder_drwav : public decoder {
        public:
            decoder_drwav();
            ~decoder_drwav() override;

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
            std::unique_ptr <impl> m_pimpl;
    };
} // namespace Aulib

/*

Copyright (C) 2021 Nikos Chantziaras.

This file is part of SDL_audiolib.

SDL_audiolib is free software: you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

SDL_audiolib is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License
along with SDL_audiolib. If not, see <http://www.gnu.org/licenses/>.

*/
