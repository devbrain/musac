// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <musac/sdk/io_stream.hh>
#include <musac/sdk/export_musac_sdk.h>
#include <musac/sdk/types.hh>
#include <chrono>
#include <memory>

namespace musac {
    /*!
     * \brief Abstract base class for audio decoders.
     */
    class MUSAC_SDK_EXPORT decoder {
        public:
            decoder();
            virtual ~decoder();

            [[nodiscard]] bool is_open() const;
            [[nodiscard]] size_t decode(float buf[], size_t len, bool& call_again, channels_t device_channels);

            virtual void open(io_stream* rwops) = 0;
            [[nodiscard]] virtual channels_t get_channels() const = 0;
            [[nodiscard]] virtual sample_rate_t get_rate() const = 0;
            virtual bool rewind() = 0;
            [[nodiscard]] virtual std::chrono::microseconds duration() const = 0;
            virtual bool seek_to_time(std::chrono::microseconds pos) = 0;

        protected:
            void set_is_open(bool f);
            virtual size_t do_decode(float* buf, size_t len, bool& call_again) = 0;

        private:
            struct impl;
            const std::unique_ptr <impl> m_pimpl;
    };


} // namespace Aulib

