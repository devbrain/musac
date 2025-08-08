#include <musac/codecs/decoder_drflac.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>

#include <musac/sdk/io_stream.hh>

#define DR_FLAC_NO_STDIO
#define DR_FLAC_IMPLEMENTATION
#define DRFLAC_API static
#include "musac/codecs/dr_libs/dr_flac.h"


namespace chrono = std::chrono;

extern "C" {
static size_t drflacReadCb(void* const rwops, void* const dst, const size_t len) {
    return static_cast <musac::io_stream*>(rwops)->read( dst, len);
}

static drflac_bool32 drflacSeekCb(void* const rwops_void, const int offset, const drflac_seek_origin origin) {
    // SDL_ClearError() removed - no longer needed

    auto* const rwops = static_cast <musac::io_stream*>(rwops_void);
    const auto rwops_size = rwops->get_size();
    const auto cur_pos = rwops->tell();

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

    musac::seek_origin whence;
    switch (origin) {
        case drflac_seek_origin_start:
            whence = musac::seek_origin::set;
            break;
        case drflac_seek_origin_current:
            whence = musac::seek_origin::cur;
            break;
        default:
            return false;
    }
    return !seekIsPastEof() && rwops->seek( offset, whence) >= 0;
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

    void decoder_drflac::open(io_stream* const rwops) {
        if (is_open()) {
            return;
        }

        m_pimpl->m_handle = {drflac_open(drflacReadCb, drflacSeekCb, rwops, nullptr), drflac_close};
        if (!m_pimpl->m_handle) {
            THROW_RUNTIME("drflac_open failed");
        }
        set_is_open(true);
    }

    size_t decoder_drflac::do_decode(float* const buf, size_t len, bool& /*callAgain*/) {
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

    channels_t decoder_drflac::get_channels() const {
        if (!is_open()) {
            return 0;
        }
        return (unsigned int)(m_pimpl->m_handle->channels);
    }

    sample_rate_t decoder_drflac::get_rate() const {
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
