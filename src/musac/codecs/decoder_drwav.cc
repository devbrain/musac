#include <musac/codecs/decoder_drwav.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>

#include <musac/sdk/io_stream.hh>

#define DR_WAV_NO_STDIO
#define DR_WAV_IMPLEMENTATION
#define DRWAV_API static
#define DRWAV_PRIVATE static
#include "musac/codecs/dr_libs/dr_wav.h"


namespace chrono = std::chrono;

extern "C" {
static size_t drwav_read_callback(void* const rwops, void* const dst, const size_t len) {
    return static_cast <musac::io_stream*>(rwops)->read( dst, len);
}

static drwav_bool32 drwav_seek_callback(void* const rwops_void, const int offset, const drwav_seek_origin origin) {
    // SDL_ClearError() removed - no longer needed

    auto* const rwops = static_cast <musac::io_stream*>(rwops_void);
    const auto rwops_size = rwops->get_size();
    const auto cur_pos = rwops->tell();

    auto seekIsPastEof = [=] {
        const auto abs_offset = (int64_t)(offset + (origin == drwav_seek_origin_current ? cur_pos : 0));
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
        case drwav_seek_origin_start:
            whence = musac::seek_origin::set;
            break;
        case drwav_seek_origin_current:
            whence = musac::seek_origin::cur;
            break;
        default:
            return false;
    }
    return !seekIsPastEof() && rwops->seek( offset, whence) >= 0;
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

    bool decoder_drwav::accept(io_stream* rwops) {
        if (!rwops) {
            return false;
        }
        
        // Save current stream position
        auto original_pos = rwops->tell();
        if (original_pos < 0) {
            return false;
        }
        
        // Use dr_wav's init function to test if the format is valid
        drwav test_handle;
        bool result = drwav_init(&test_handle, drwav_read_callback, drwav_seek_callback, rwops, nullptr);
        if (result) {
            drwav_uninit(&test_handle);
        }
        
        // Restore original position
        rwops->seek(original_pos, seek_origin::set);
        return result;
    }
    
    const char* decoder_drwav::get_name() const {
        return "WAV (dr_wav)";
    }

    void decoder_drwav::open(io_stream* const rwops) {
        if (is_open()) {
            return;
        }

        if (!drwav_init(&m_pimpl->m_handle, drwav_read_callback, drwav_seek_callback, rwops, nullptr)) {
            THROW_RUNTIME("drwav_init failed");
        }
        set_is_open(true);
    }

    size_t decoder_drwav::do_decode(float* const buf, size_t len, bool& callAgain) {
        if (m_pimpl->m_eof || !is_open()) {
            callAgain = false;
            return 0;
        }

        const auto ret =
            drwav_read_pcm_frames_f32(&m_pimpl->m_handle, len / get_channels(), buf) * get_channels();
        if (ret < static_cast <drwav_uint64>(len)) {
            m_pimpl->m_eof = true;
            callAgain = false;
        } else {
            callAgain = true;
        }
        return static_cast<unsigned int>(ret);
    }

    channels_t decoder_drwav::get_channels() const {
        return m_pimpl->m_handle.channels;
    }

    sample_rate_t decoder_drwav::get_rate() const {
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
