// This is copyrighted software. More information is at the end of this file.
#include <musac/sdk/resampler.hh>

#include <musac/sdk/decoder.hh>
#include <musac/sdk/buffer.hh>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

static void relocate_buffer(float* buf, std::size_t& pos, std::size_t& end) {
    if (end < 1) {
        return;
    }
    if (pos >= end) {
        pos = end = 0;
        return;
    }
    if (pos < 1) {
        return;
    }
    auto len = end - pos;
    memmove(buf, buf + pos, static_cast <size_t>(len) * sizeof(*buf));
    pos = 0;
    end = len;
}

namespace musac {
    struct resampler::impl final {
        resampler* m_owner;

        explicit impl(resampler* pub);

        std::shared_ptr <decoder> m_decoder = nullptr;
        unsigned int m_dst_rate = 0;
        unsigned int m_src_rate = 0;
        unsigned int m_channels = 0;
        unsigned int m_chunk_size = 0;
        buffer <float> m_out_buffer{0};
        buffer <float> m_input_buffer{0};
        std::size_t m_out_buffer_pos = 0;
        std::size_t m_out_buffer_end = 0;
        std::size_t m_in_buffer_pos = 0;
        std::size_t m_in_buffer_end = 0;
        bool m_pending_spec_change = false;

        /* Move at most 'dstLen' samples from the output buffer into 'dst'.
         *
         * Returns the amount of samples that were actually moved.
         */
        std::size_t move_from_out_buffer(float dst[], std::size_t dst_len);

        /* Adjust all internal buffer sizes for the current source and target
         * sampling rates.
         */
        void adjust_buffer_sizes();

        /* Resample samples from the input buffer and move them to the output
         * buffer.
         */
        void resample_from_in_buffer();
    };

    resampler::impl::impl(resampler* pub)
        : m_owner(pub) {
    }

    std::size_t resampler::impl::move_from_out_buffer(float dst[], std::size_t dst_len) {
        if (m_out_buffer_end == 0) {
            return 0;
        }
        if (m_out_buffer_pos >= m_out_buffer_end) {
            m_out_buffer_pos = m_out_buffer_end = 0;
            return 0;
        }
        auto len = std::min(m_out_buffer_end - m_out_buffer_pos, dst_len);
        memcpy(dst, m_out_buffer.data() + m_out_buffer_pos,
               static_cast <size_t>(len) * sizeof(float));
        m_out_buffer_pos += len;
        if (m_out_buffer_pos >= m_out_buffer_end) {
            m_out_buffer_end = m_out_buffer_pos = 0;
        }
        return len;
    }

    void resampler::impl::adjust_buffer_sizes() {
        auto old_in_buf_len = m_in_buffer_end - m_in_buffer_pos;
        unsigned int out_buf_siz = m_channels * m_chunk_size;
        unsigned int in_buf_siz;

        if (m_dst_rate == m_src_rate) {
            // In the no-op case where we don't actually resample, input and output
            // buffers have the same size, since we're just copying the samples
            // as-is from input to output.
            in_buf_siz = out_buf_siz;
        } else {
            // When resampling, the input buffer's size depends on the ratio between
            // the source and destination sample rates.
            in_buf_siz = static_cast <unsigned int>(std::ceil(
                static_cast <float>(out_buf_siz) * (float)m_src_rate / (float)m_dst_rate));
            auto remainder = in_buf_siz % m_channels;
            if (remainder != 0) {
                in_buf_siz = in_buf_siz + m_channels - remainder;
            }
        }

        m_out_buffer.reset(out_buf_siz);
        m_input_buffer.resize(in_buf_siz);
        m_out_buffer_pos = m_out_buffer_end = m_in_buffer_pos = 0;
        if (old_in_buf_len != 0) {
            m_in_buffer_end = old_in_buf_len;
        } else {
            m_in_buffer_end = 0;
        }
    }

    void resampler::impl::resample_from_in_buffer() {
        auto in_len = m_in_buffer_end - m_in_buffer_pos;
        float* from = m_input_buffer.data() + m_in_buffer_pos;
        float* to = m_out_buffer.data() + m_out_buffer_end;
        if (m_src_rate == m_dst_rate) {
            // No resampling is needed. Just copy the samples as-is.
            auto outLen = std::min(m_out_buffer.size() - m_out_buffer_end, in_len);
            std::memcpy(to, from, static_cast <size_t>(outLen) * sizeof(float));
            m_out_buffer_end += outLen;
            m_in_buffer_pos += outLen;
        } else {
            auto outLen = m_out_buffer.size() - m_out_buffer_end;
            m_owner->do_resampling(to, from, outLen, in_len);
            m_out_buffer_end += outLen;
            m_in_buffer_pos += in_len;
        }
        if (m_in_buffer_pos >= m_in_buffer_end) {
            // No more samples left to resample. Mark the input buffer as empty.
            m_in_buffer_pos = m_in_buffer_end = 0;
        }
    }

    resampler::resampler()
        : m_pimpl(std::make_unique <impl>(this)) {
    }

    resampler::~resampler() = default;

    void resampler::set_decoder(std::shared_ptr <decoder> decoder) {
        m_pimpl->m_decoder = std::move(decoder);
    }

    int resampler::set_spec(sample_rate_t dst_rate, channels_t channels, size_t chunk_size) {
        m_pimpl->m_dst_rate = dst_rate;
        m_pimpl->m_channels = channels;
        m_pimpl->m_chunk_size = chunk_size;
        m_pimpl->m_src_rate = m_pimpl->m_decoder->get_rate();
        m_pimpl->m_src_rate = std::min(std::max(4000u, m_pimpl->m_src_rate), 192000u);
        m_pimpl->adjust_buffer_sizes();
        // Inform our child class about the spec change.
        adjust_for_output_spec(m_pimpl->m_dst_rate, m_pimpl->m_src_rate, m_pimpl->m_channels);
        return 0;
    }

    unsigned int resampler::get_current_rate() const {
        return m_pimpl->m_dst_rate;
    }

    unsigned int resampler::get_current_channels() const {
        return m_pimpl->m_channels;
    }

    unsigned int resampler::get_current_chunk_size() const {
        return m_pimpl->m_chunk_size;
    }

    std::size_t resampler::resample(float dst[], std::size_t dst_len) {
        unsigned int total_samples = 0;
        bool dec_eof = false;

        if (m_pimpl->m_pending_spec_change) {
            // There's a spec change pending. Process any data that is still in our
            // buffers using the current spec.
            total_samples += m_pimpl->move_from_out_buffer(dst, dst_len);
            relocate_buffer(m_pimpl->m_out_buffer.data(), m_pimpl->m_out_buffer_pos, m_pimpl->m_out_buffer_end);
            m_pimpl->resample_from_in_buffer();
            if (total_samples >= dst_len) {
                // There's still samples left in the output buffer, so don't change
                // the spec yet.
                return dst_len;
            }
            // Our buffers are empty, so we can change to the new spec.
            set_spec(m_pimpl->m_dst_rate, m_pimpl->m_channels, m_pimpl->m_chunk_size);
            m_pimpl->m_pending_spec_change = false;
        }

        // Keep resampling until we either produce the requested amount of output
        // samples, or the decoder has no more samples to give us.
        while (total_samples < dst_len && !dec_eof) {
            // If the input buffer is not filled, get some more samples from the
            // decoder.
            if (m_pimpl->m_in_buffer_end < m_pimpl->m_input_buffer.size()) {
                bool call_again = false;
                auto dec_samples = m_pimpl->m_decoder->decode(m_pimpl->m_input_buffer.data() + m_pimpl->m_in_buffer_end,
                                                              m_pimpl->m_input_buffer.size() - m_pimpl->m_in_buffer_end,
                                                              call_again, m_pimpl->m_channels);
                // If the decoder indicated a spec change, process any data that is
                // still in our buffers using the current spec.
                if (call_again) {
                    m_pimpl->m_in_buffer_end += dec_samples;
                    m_pimpl->resample_from_in_buffer();
                    total_samples += m_pimpl->move_from_out_buffer(dst + total_samples, dst_len - total_samples);
                    if (total_samples >= dst_len) {
                        // There's still samples left in the output buffer. Keep
                        // the current spec and prepare to change it on our next
                        // call.
                        m_pimpl->m_pending_spec_change = true;
                        return dst_len;
                    }
                    set_spec(m_pimpl->m_dst_rate, m_pimpl->m_channels, m_pimpl->m_chunk_size);
                } else if (dec_samples <= 0) {
                    dec_eof = true;
                } else {
                    m_pimpl->m_in_buffer_end += dec_samples;
                }
            }

            m_pimpl->resample_from_in_buffer();
            relocate_buffer(m_pimpl->m_input_buffer.data(), m_pimpl->m_in_buffer_pos, m_pimpl->m_in_buffer_end);
            total_samples += m_pimpl->move_from_out_buffer(dst + total_samples, dst_len - total_samples);
            relocate_buffer(m_pimpl->m_out_buffer.data(), m_pimpl->m_out_buffer_pos, m_pimpl->m_out_buffer_end);
        }
        return total_samples;
    }

    void resampler::discard_pending_samples() {
        m_pimpl->m_out_buffer_pos = m_pimpl->m_out_buffer_end = m_pimpl->m_in_buffer_pos = m_pimpl->m_in_buffer_end = 0;
        do_discard_pending_samples();
    }
}
