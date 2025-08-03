//
// Created by igor on 3/23/25.
//

#ifndef  AUDIO_SOURCE_HH
#define  AUDIO_SOURCE_HH

#include <chrono>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/resampler.hh>
#include <musac/export_musac.h>

namespace musac {
    class MUSAC_EXPORT audio_source {
        public:
            audio_source(std::unique_ptr <decoder> decoder_obj,
                         std::unique_ptr <resampler> resampler_obj,
                         musac::io_stream* rwops,
                         bool do_close);

            audio_source(std::unique_ptr <decoder> decoder_obj,
                         musac::io_stream* rwops,
                         bool do_close);

            audio_source(audio_source&&) noexcept;

            audio_source(const audio_source&) = delete;
            audio_source& operator = (const audio_source&) = delete;

            ~audio_source();

            bool rewind();

            bool open(unsigned int rate, unsigned int channels, unsigned int frame_size);

            void read_samples(float* buf, unsigned int& cur_pos, unsigned int len, unsigned int device_channels);

            std::chrono::microseconds duration() const;
            bool seek_to_time(std::chrono::microseconds pos) const;
        private:
            musac::io_stream* m_rwops;
            bool m_close_rw;
            // Resamplers hold a reference to decoders, so we store it as a shared_ptr.
            std::shared_ptr <decoder> m_decoder;
            std::unique_ptr <resampler> m_resampler;
    };
}

#endif
