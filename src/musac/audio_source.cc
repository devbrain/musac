//
// Created by igor on 3/23/25.
//

#include <musac/audio_source.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include <iostream>
musac::audio_source::audio_source(std::unique_ptr <decoder> decoder_obj,
                                  std::unique_ptr <resampler> resampler_obj,
                                  std::unique_ptr<musac::io_stream> rwops)
    : m_rwops(std::move(rwops)),
      m_decoder(std::move(decoder_obj)),
      m_resampler(std::move(resampler_obj)) {
    if (m_resampler) {
        m_resampler->set_decoder(m_decoder);
    }
}

musac::audio_source::audio_source(std::unique_ptr <decoder> decoder_obj, std::unique_ptr<musac::io_stream> rwops)
    : m_rwops(std::move(rwops)),
      m_decoder(std::move(decoder_obj)) {
}

musac::audio_source::audio_source(audio_source&& other) noexcept
    : m_rwops(std::move(other.m_rwops)),
      m_decoder(std::move(other.m_decoder)),
      m_resampler(std::move(other.m_resampler)) {
}

musac::audio_source::~audio_source() {
    // Destructor will automatically clean up unique_ptr members
}

bool musac::audio_source::rewind() {
    return m_decoder->rewind();
}

void musac::audio_source::open(sample_rate_t rate, channels_t channels, size_t frame_size) {
    if (!m_rwops) {
        THROW_RUNTIME("No IO stream available for audio source");
    }
    
    try {
        m_decoder->open(m_rwops.get());
    } catch (const std::exception& e) {
        // Re-throw with more context
        THROW_RUNTIME("Failed to open audio decoder: ", e.what());
    }
    
    if (m_resampler) {
        m_resampler->set_spec(rate, channels, frame_size);
    }
}

void musac::audio_source::read_samples(float buf[], size_t& cur_pos, size_t len, channels_t device_channels) {
    if (m_resampler) {
        cur_pos += m_resampler->resample(buf + cur_pos, len - cur_pos);
    } else {
        bool call_again = false;
        do {
            call_again = false;
            cur_pos += m_decoder->decode(buf + cur_pos, len - cur_pos, call_again, device_channels);
        }
        while (cur_pos < len && call_again);
    }
}

std::chrono::microseconds musac::audio_source::duration() const {
    return m_decoder->duration();
}

bool musac::audio_source::seek_to_time(std::chrono::microseconds pos) const {
    return m_decoder->seek_to_time(pos);
}
