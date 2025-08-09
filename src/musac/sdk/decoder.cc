// This is copyrighted software. More information is at the end of this file.
#include <musac/sdk/decoder.hh>

#include <memory>

#include <musac/sdk/buffer.hh>


namespace musac {
    struct decoder::impl final {
        buffer <float> m_stereo_buf{0};
        bool m_is_open = false;
    };

    decoder::decoder()
        : m_pimpl(std::make_unique <impl>()) {
    }

    decoder::~decoder() = default;

    bool decoder::is_open() const {
        return m_pimpl->m_is_open;
    }

    bool decoder::accept(io_stream* rwops) {
        if (!rwops) {
            return false;
        }
        
        // Save current stream position
        auto original_pos = rwops->tell();
        if (original_pos < 0) {
            return false;
        }
        
        bool result = false;
        try {
            // Call the derived class implementation
            result = do_accept(rwops);
        } catch (...) {
            // If do_accept throws, treat it as format not accepted
            result = false;
        }
        
        // Always restore original position
        rwops->seek(original_pos, seek_origin::set);
        
        return result;
    }

    // Conversion happens in-place.
    static constexpr void mono_to_stereo(float buf[], size_t len) {
        if (len < 1 || !buf) {
            return;
        }
        for (int i = (int)len / 2 - 1, j = (int)len - 1; i >= 0; --i) {
            buf[j--] = buf[i];
            buf[j--] = buf[i];
        }
    }

    static constexpr void stereo_to_mono(float dst[], const float src[], size_t src_len) {
        if (src_len < 1 || !dst || !src) {
            return;
        }
        for (size_t i = 0, j = 0; i < src_len; i += 2, ++j) {
            dst[j] = src[i] * 0.5f;
            dst[j] += src[i + 1] * 0.5f;
        }
    }

    size_t decoder::decode(float buf[], size_t len, bool& call_again, channels_t device_channels) {
        if (this->get_channels() == 1 && device_channels == 2) {
            auto srcLen = this->do_decode(buf, len / 2, call_again);
            mono_to_stereo(buf, srcLen * 2);
            return srcLen * 2;
        }

        if (this->get_channels() == 2 && device_channels == 1) {
            if (m_pimpl->m_stereo_buf.size() != len * 2) {
                m_pimpl->m_stereo_buf.reset(len * 2);
            }
            auto srcLen = this->do_decode(m_pimpl->m_stereo_buf.data(), m_pimpl->m_stereo_buf.size(), call_again);
            stereo_to_mono(buf, m_pimpl->m_stereo_buf.data(), srcLen);
            return srcLen / 2;
        }
        return this->do_decode(buf, len, call_again);
    }

    void decoder::set_is_open(bool f) {
        m_pimpl->m_is_open = f;
    }
}
