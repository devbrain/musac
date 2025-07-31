// This is copyrighted software. More information is at the end of this file.
#include <musac/codecs/decoder_drmp3.hh>


#define DRMP3_API static
#define DR_MP3_NO_STDIO
#define DR_MP3_IMPLEMENTATION

#include "musac/codecs/dr_libs/dr_mp3.h"

namespace chrono = std::chrono;

extern "C" {
static size_t drmp3ReadCb(void* const rwops, void* const dst, const size_t len) {
    return SDL_ReadIO(static_cast <SDL_IOStream*>(rwops), dst, len);
}

static drmp3_bool32 drmp3SeekCb(void* const rwops_void, const int offset, const drmp3_seek_origin origin) {
    SDL_ClearError();

    auto* const rwops = static_cast <SDL_IOStream*>(rwops_void);
    const auto rwops_size = SDL_GetIOSize(rwops);
    const auto cur_pos = SDL_TellIO(rwops);

    auto seekIsPastEof = [=] {
        const auto abs_offset = offset + (origin == drmp3_seek_origin_current ? cur_pos : 0);
        return abs_offset >= rwops_size;
    };

    if (rwops_size < 0) {
        return false;
    }
    if (cur_pos < 0) {
        return false;
    }

    SDL_IOWhence whence;
    switch (origin) {
        case drmp3_seek_origin_start:
            whence = SDL_IO_SEEK_SET;
            break;
        case drmp3_seek_origin_current:
            whence = SDL_IO_SEEK_CUR;
            break;
        default:
            return false;
    }
    return !seekIsPastEof() && SDL_SeekIO(rwops, offset, whence) >= 0;
}
} // extern "C"

namespace musac {
    struct decoder_drmp3::impl final {
        drmp3 handle_{};
        std::chrono::microseconds duration_{};
        bool fEOF = false;
    };

    decoder_drmp3::decoder_drmp3()
        : m_pimpl(std::make_unique <impl>()) {
    }

    decoder_drmp3::~decoder_drmp3() {
        if (!is_open()) {
            return;
        }
        drmp3_uninit(&m_pimpl->handle_);
    }

    bool decoder_drmp3::open(SDL_IOStream* const rwops) {
        if (is_open()) {
            return true;
        }

        if (!drmp3_init(&m_pimpl->handle_, drmp3ReadCb, drmp3SeekCb, rwops, nullptr)) {
            SDL_SetError("drmp3_init failed.");
            return false;
        }
        // Calculating the duration on an MP3 stream involves iterating over every frame in it, which is
        // only possible when the total size of the stream is known.
        if (SDL_GetIOSize(rwops) > 0) {
            m_pimpl->duration_ = chrono::duration_cast <chrono::microseconds>(chrono::duration <double>(
                static_cast <double>(drmp3_get_pcm_frame_count(&m_pimpl->handle_)) / get_rate()));
        }
        set_is_open(true);
        return true;
    }

    unsigned int decoder_drmp3::do_decode(float* const buf, unsigned int len, bool& /*callAgain*/) {
        if (m_pimpl->fEOF || !is_open()) {
            return 0;
        }

        const auto ret =
            drmp3_read_pcm_frames_f32(&m_pimpl->handle_, len / get_channels(), buf) * get_channels();
        if (ret < static_cast <drmp3_uint64>(len)) {
            m_pimpl->fEOF = true;
        }
        return static_cast <unsigned int>(ret);
    }

    unsigned int decoder_drmp3::get_channels() const {
        return m_pimpl->handle_.channels;
    }

    unsigned int decoder_drmp3::get_rate() const {
        return m_pimpl->handle_.sampleRate;
    }

    bool decoder_drmp3::rewind() {
        return seek_to_time({});
    }

    chrono::microseconds decoder_drmp3::duration() const {
        return m_pimpl->duration_;
    }

    bool decoder_drmp3::seek_to_time(const chrono::microseconds pos) {
        if (!is_open()) {
            return false;
        }

        const auto target_frame = chrono::duration <double>(pos).count() * get_rate();
        if (!drmp3_seek_to_pcm_frame(&m_pimpl->handle_, (drmp3_uint64)target_frame)) {
            return false;
        }
        m_pimpl->fEOF = false;
        return true;
    }
}


