#include <musac/codecs/decoder_drflac.hh>



#define DR_FLAC_NO_STDIO
#define DR_FLAC_IMPLEMENTATION
#define DRFLAC_API static
#include "codecs/dr_libs/dr_flac.h"

#include <SDL3/SDL.h>

namespace chrono = std::chrono;

extern "C" {
static size_t drflacReadCb(void* const rwops, void* const dst, const size_t len) {
    return SDL_ReadIO(static_cast <SDL_IOStream*>(rwops), dst, len);
}

static drflac_bool32 drflacSeekCb(void* const rwops_void, const int offset, const drflac_seek_origin origin) {
    SDL_ClearError();

    auto* const rwops = static_cast <SDL_IOStream*>(rwops_void);
    const auto rwops_size = SDL_GetIOSize(rwops);
    const auto cur_pos = SDL_TellIO(rwops);

    auto seekIsPastEof = [=] {
        const auto abs_offset = offset + (origin == drflac_seek_origin_current ? cur_pos : 0);
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
        case drflac_seek_origin_start:
            whence = SDL_IO_SEEK_SET;
            break;
        case drflac_seek_origin_current:
            whence = SDL_IO_SEEK_CUR;
            break;
        default:
            return false;
    }
    return !seekIsPastEof() && SDL_SeekIO(rwops, offset, whence) >= 0;
}
} // extern "C"

namespace musac {
    struct decoder_drflac::impl final {
        std::unique_ptr <drflac, decltype(&drflac_close)> m_handle{nullptr, drflac_close};
        bool m_eof = false;
    };

    decoder_drflac::decoder_drflac()
        : m_pimpl(std::make_unique <impl>()) {
    }

    decoder_drflac::~decoder_drflac() = default;

    bool decoder_drflac::open(SDL_IOStream* const rwops) {
        if (is_open()) {
            return true;
        }

        m_pimpl->m_handle = {drflac_open(drflacReadCb, drflacSeekCb, rwops, nullptr), drflac_close};
        if (!m_pimpl->m_handle) {
            SDL_SetError("drflac_open returned null.");
            return false;
        }
        set_is_open(true);
        return true;
    }

    unsigned int decoder_drflac::do_decode(float* const buf, unsigned int len, bool& /*callAgain*/) {
        if (m_pimpl->m_eof || !is_open()) {
            return 0;
        }

        const auto ret =
            drflac_read_pcm_frames_f32(m_pimpl->m_handle.get(), len / get_channels(), buf) * get_channels();
        if (ret < static_cast <drflac_uint64>(len)) {
            m_pimpl->m_eof = true;
        }
        return (unsigned int)ret;
    }

    unsigned int decoder_drflac::get_channels() const {
        if (!is_open()) {
            return 0;
        }
        return (unsigned int)(m_pimpl->m_handle->channels);
    }

    unsigned int decoder_drflac::get_rate() const {
        if (!is_open()) {
            return 0;
        }
        return m_pimpl->m_handle->sampleRate;
    }

    bool decoder_drflac::rewind() {
        return seek_to_time({});
    }

    chrono::microseconds decoder_drflac::duration() const {
        if (!is_open()) {
            return {};
        }
        return chrono::duration_cast <chrono::microseconds>(chrono::duration <double>(
            static_cast <double>(m_pimpl->m_handle->totalPCMFrameCount) / get_rate()));
    }

    bool decoder_drflac::seek_to_time(const chrono::microseconds pos) {
        const auto target_frame = chrono::duration <double>(pos).count() * get_rate();
        if (!is_open() || !drflac_seek_to_pcm_frame(m_pimpl->m_handle.get(), (drflac_uint64)target_frame)) {
            return false;
        }
        m_pimpl->m_eof = false;
        return true;
    }
}
