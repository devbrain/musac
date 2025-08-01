// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <musac/sdk/sdl_compat.h>
#include <musac/sdk/decoder.hh>
#include <musac/codecs/export_musac_codecs.h>
namespace musac {
    /*!
     * \brief ModPlug decoder.
     */
    class MUSAC_CODECS_EXPORT decoder_modplug : public decoder {
        public:
            decoder_modplug();
            ~decoder_modplug() override;

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
            const std::unique_ptr <impl> m_pimpl;
    };
} // namespace Aulib

/*

Copyright (C) 2014, 2015, 2016, 2017, 2018, 2019 Nikos Chantziaras.

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
