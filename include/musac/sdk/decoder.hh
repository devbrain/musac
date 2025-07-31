// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <SDL3/SDL.h>
#include <chrono>
#include <memory>

#include <musac/sdk/export_musac_sdk.h>

namespace musac {
    /*!
     * \brief Abstract base class for audio decoders.
     */
    class MUSAC_SDK_EXPORT decoder {
        public:
            decoder();
            virtual ~decoder();

            [[nodiscard]] bool is_open() const;
            [[nodiscard]] unsigned int decode(float buf[], unsigned int len, bool& call_again, unsigned int device_channels);

            [[nodiscard]] virtual bool open(SDL_IOStream* rwops) = 0;
            [[nodiscard]] virtual unsigned int get_channels() const = 0;
            [[nodiscard]] virtual unsigned int get_rate() const = 0;
            virtual bool rewind() = 0;
            [[nodiscard]] virtual std::chrono::microseconds duration() const = 0;
            virtual bool seek_to_time(std::chrono::microseconds pos) = 0;

        protected:
            void set_is_open(bool f);
            virtual unsigned int do_decode(float* buf, unsigned int len, bool& call_again) = 0;

        private:
            struct impl;
            const std::unique_ptr <impl> m_pimpl;
    };


} // namespace Aulib

