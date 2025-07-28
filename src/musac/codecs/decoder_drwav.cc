#include "musac/codecs/decoder_drwav.hh"


#define DR_WAV_NO_STDIO
#define DR_WAV_IMPLEMENTATION
#define DRWAV_API static
#define DRWAV_PRIVATE static
#include "codecs/dr_libs/dr_wav.h"


namespace chrono = std::chrono;

extern "C" {
static size_t drwav_read_callback(void* const rwops, void* const dst, const size_t len) {
    return SDL_ReadIO(static_cast <SDL_IOStream*>(rwops), dst, len);
}

static drwav_bool32 drwav_seek_callback(void* const rwops_void, const int offset, const drwav_seek_origin origin) {
    SDL_ClearError();

    auto* const rwops = static_cast <SDL_IOStream*>(rwops_void);
    const auto rwops_size = SDL_GetIOSize(rwops);
    const auto cur_pos = SDL_TellIO(rwops);

    auto seekIsPastEof = [=] {
        const auto abs_offset = (Sint64)(offset + (origin == drwav_seek_origin_current ? cur_pos : 0));
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
        case drwav_seek_origin_start:
            whence = SDL_IO_SEEK_SET;
            break;
        case drwav_seek_origin_current:
            whence = SDL_IO_SEEK_CUR;
            break;
        default:
            return false;
    }
    return !seekIsPastEof() && SDL_SeekIO(rwops, offset, whence) >= 0;
}
} // extern "C"

namespace musac {
    struct decoder_drwav::impl final {
        drwav m_handle{};
        bool m_eof = false;
    };

    decoder_drwav::decoder_drwav()
        : m_pimpl(std::make_unique <impl>()) {
    }

    decoder_drwav::~decoder_drwav() {
        if (!is_open()) {
            return;
        }
        drwav_uninit(&m_pimpl->m_handle);
    }

    auto decoder_drwav::open(SDL_IOStream* const rwops)
        ->
        bool {
        if (is_open()) {
            return true;
        }

        if (!drwav_init(&m_pimpl->m_handle, drwav_read_callback, drwav_seek_callback, rwops, nullptr)) {
            SDL_SetError("drwav_init failed.");
            return false;
        }
        set_is_open(true);
        return true;
    }

    unsigned int decoder_drwav::do_decode(float* const buf, unsigned int len, bool& /*callAgain*/) {
        if (m_pimpl->m_eof || !is_open()) {
            return 0;
        }

        const auto ret =
            drwav_read_pcm_frames_f32(&m_pimpl->m_handle, len / get_channels(), buf) * get_channels();
        if (ret < static_cast <drwav_uint64>(len)) {
            m_pimpl->m_eof = true;
        }
        return static_cast<unsigned int>(ret);
    }

    unsigned int decoder_drwav::get_channels() const {
        return m_pimpl->m_handle.channels;
    }

    unsigned int decoder_drwav::get_rate() const {
        return m_pimpl->m_handle.sampleRate;
    }

    bool decoder_drwav::rewind() {
        return seek_to_time({});
    }

    chrono::microseconds decoder_drwav::duration() const {
        if (!is_open()) {
            return {};
        }
        return chrono::duration_cast <chrono::microseconds>(
            chrono::duration <double>(static_cast <double>(m_pimpl->m_handle.totalPCMFrameCount) / get_rate()));
    }

    bool decoder_drwav::seek_to_time(const chrono::microseconds pos) {
        const auto target_frame = chrono::duration <double>(pos).count() * get_rate();
        if (!is_open() || !drwav_seek_to_pcm_frame(&m_pimpl->m_handle, (drwav_uint64)target_frame)) {
            return false;
        }
        m_pimpl->m_eof = false;
        return true;
    }
}
