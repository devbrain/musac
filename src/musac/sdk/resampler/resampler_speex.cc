// This is copyrighted software. More information is at the end of this file.
#include "resampler_speex.hh"
#include <musac/sdk/decoder.hh>
#define OUTSIDE_SPEEX
#define RANDOM_PREFIX zopa_
#include "speex/speex_resampler.h"

#include <algorithm>

namespace musac {
    struct resampler_speex::impl final {
        explicit impl(int quality)
            : m_quality(quality) {
        }

        using resampler_ptr_t = std::unique_ptr <SpeexResamplerState, decltype(&speex_resampler_destroy)>;
        resampler_ptr_t m_resampler{nullptr, &speex_resampler_destroy};
        unsigned int m_src_rate = 0;
        int m_quality;
    };

    resampler_speex::resampler_speex(int quality)
        : m_pimpl(std::make_unique <impl>(std::min(std::max(0, quality), 10))) {
    }

    resampler_speex::~resampler_speex() = default;

    auto resampler_speex::quality() const noexcept -> int {
        return m_pimpl->m_quality;
    }

    void resampler_speex::set_quality(int quality) {
        auto newQ = std::min(std::max(0, quality), 10);
        m_pimpl->m_quality = newQ;
        if (!m_pimpl->m_resampler) {
            return;
        }
        speex_resampler_set_quality(m_pimpl->m_resampler.get(), newQ);
    }

    void resampler_speex::do_resampling(float dst[], const float src[], std::size_t& dst_len, std::size_t& src_len) {
        if (!m_pimpl->m_resampler) {
            dst_len = src_len = 0;
            return;
        }

        unsigned int channels = get_current_channels();
        auto spxInLen = static_cast<unsigned int>(src_len / channels);
        auto spxOutLen = static_cast<unsigned int>(dst_len / channels);
        if (spxInLen == 0 || spxOutLen == 0) {
            dst_len = src_len = 0;
            return;
        }
        speex_resampler_process_interleaved_float(m_pimpl->m_resampler.get(), src, &spxInLen, dst, &spxOutLen);
        dst_len = spxOutLen * channels;
        src_len = spxInLen * channels;
    }

    auto resampler_speex::adjust_for_output_spec(uint32_t dst_rate, uint32_t src_rate, uint8_t channels) -> int {
        int err;
        m_pimpl->m_resampler.reset(speex_resampler_init(
            channels, src_rate,
            dst_rate, m_pimpl->m_quality, &err));
        if (err != 0) {
            m_pimpl->m_resampler = nullptr;
            return -1;
        }
        m_pimpl->m_src_rate = src_rate;
        return 0;
    }

    void resampler_speex::do_discard_pending_samples() {
        // The speex resampler does not offer a way to clear its internal state.
        // speex_resampler_reset_mem() does not work. We are forced to allocate a new resampler handle.
        if (m_pimpl->m_resampler) {
            adjust_for_output_spec(get_current_rate(), m_pimpl->m_src_rate, get_current_channels());
        }
    }
}
