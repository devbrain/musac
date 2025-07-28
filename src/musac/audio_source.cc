//
// Created by igor on 3/23/25.
//

#include "../../include/musac/audio_source.hh"

musac::audio_source::audio_source(std::unique_ptr <decoder> decoder_obj,
                                  std::unique_ptr <resampler> resampler_obj,
                                  SDL_IOStream* rwops, bool do_close)
    : m_rwops(rwops),
      m_close_rw(do_close),
      m_decoder(std::move(decoder_obj)),
      m_resampler(std::move(resampler_obj)) {
    if (m_resampler) {
        m_resampler->set_decoder(m_decoder);
    }
}

musac::audio_source::audio_source(std::unique_ptr <decoder> decoder_obj, SDL_IOStream* rwops, bool do_close)
    : m_rwops(rwops),
      m_close_rw(do_close),
      m_decoder(std::move(decoder_obj)) {
}

musac::audio_source::audio_source(audio_source&& other) noexcept
    : m_rwops(other.m_rwops),
      m_close_rw(other.m_close_rw),
      m_decoder(std::move(other.m_decoder)) {
    other.m_rwops = nullptr;
    other.m_close_rw = false;
}

musac::audio_source::~audio_source() {
    if (m_close_rw && m_rwops) {
        SDL_CloseIO(m_rwops);
    }
}

bool musac::audio_source::rewind() {
    return m_decoder->rewind();
}

bool musac::audio_source::open(unsigned int rate, unsigned int channels, unsigned int frame_size) {
    if (!m_rwops) {
        return false;
    }
    if (!m_decoder->open(m_rwops)) {
        return false;
    }
    if (m_resampler) {
        m_resampler->set_spec(rate, channels, frame_size);
    }
    return true;
}

void musac::audio_source::read_samples(float buf[], unsigned int& cur_pos, unsigned int len, unsigned int device_channels) {
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
