//
// Created by igor on 4/28/25.
//

#include "musac/audio_mixer.hh"
#include <musac/stream.hh>
#include <failsafe/failsafe.hh>
#include <algorithm>
namespace musac {
    audio_device_data audio_mixer::m_audio_device_data {};

    audio_mixer::audio_mixer(): m_final_mix_buf(0),
                                m_stream_buf(0),
                                m_processor_buf(0) {
    }

    std::shared_ptr<std::vector<stream_container::stream_entry>> audio_mixer::get_streams() const {
        return m_stream_container.get_valid_streams();
    }

    void audio_mixer::add_stream(audio_stream* s, std::weak_ptr<void> lifetime_token) {
        if (!s) return;
        m_stream_container.add(s, lifetime_token, s->get_token());
    }

    void audio_mixer::remove_stream(int token) {
        m_stream_container.remove(token);
    }

    void audio_mixer::resize(unsigned int out_len_samples) {
        // Only grow buffers; never shrink
        if (out_len_samples > m_allocated_samples) {
            m_final_mix_buf.resize(out_len_samples);
            m_stream_buf.resize(out_len_samples);
            m_processor_buf.resize(out_len_samples);
            m_allocated_samples = out_len_samples;
        }
        // if out_len_samples <= m_allocated_samples, keep existing buffers
    }

    void audio_mixer::set_zeros() {
        std::fill(m_final_mix_buf.begin(), m_final_mix_buf.end(), 0.f);
    }

    // Define a portable IVDEP hint macro
#ifndef PRAGMA_IVDEP
#if defined(_MSC_VER)
#define PRAGMA_IVDEP __pragma(loop(ivdep))
#elif defined(__INTEL_COMPILER)
#define PRAGMA_IVDEP _Pragma("ivdep")
#elif defined(__clang__)
    // Clang’s vectorization hint—you can tweak to your needs
#define PRAGMA_IVDEP _Pragma("clang loop vectorize(enable) interleave(enable)")
#elif defined(__GNUC__)
#define PRAGMA_IVDEP _Pragma("GCC ivdep")
#else
#define PRAGMA_IVDEP
#endif
#endif


    void audio_mixer::mix_channels(unsigned int channels, unsigned int out_offset, unsigned int cur_pos,
        float volume_left, float volume_right) {
        float* dst = m_final_mix_buf.data() + out_offset;
        float* src = m_stream_buf.data() + out_offset;
        const size_t total_samples = cur_pos - out_offset;

        if (channels > 1) {
            // Stereo interleaved
            const size_t frames = total_samples / 2;
            float* d = dst;
            float* s = src;
            if (volume_left == 1.f && volume_right == 1.f) {
                PRAGMA_IVDEP
                for (size_t i = 0; i < frames; ++i) {
                    d[0] += s[0];
                    d[1] += s[1];
                    d += 2;
                    s += 2;
                }
            } else {
                PRAGMA_IVDEP
                for (size_t i = 0; i < frames; ++i) {
                    d[0] += s[0] * volume_left;
                    d[1] += s[1] * volume_right;
                    d += 2;
                    s += 2;
                }
            }
        } else {
            // Mono
            float* d = dst;
            float* s = src;
            if (volume_left == 1.f) {
                PRAGMA_IVDEP
                for (size_t i = 0; i < total_samples; ++i) {
                    d[i] += s[i];
                }
            } else {
                PRAGMA_IVDEP
                for (size_t i = 0; i < total_samples; ++i) {
                    d[i] += s[i] * volume_left;
                }
            }
        }
    }

    float* audio_mixer::finalMixData() {
        return m_final_mix_buf.data();
    }

    unsigned int audio_mixer::allocatedSamples() const {
        return m_allocated_samples;
    }
}
