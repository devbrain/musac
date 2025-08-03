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
        // Phase 3: No need to allocate shared_ptr, using direct vector with mutex
    }

    std::shared_ptr<std::vector<audio_stream*>> audio_mixer::get_streams() const {
        // Phase 3: Return a copy under read lock for safe iteration
        std::shared_lock lock(m_streams_mutex);
        return std::make_shared<std::vector<audio_stream*>>(m_streams);
    }

    void audio_mixer::add_stream(audio_stream* s) {
        // Phase 3: Use write lock for modifications
        std::unique_lock lock(m_streams_mutex);
        
        // Check if stream is already in the list
        auto it = std::find(m_streams.begin(), m_streams.end(), s);
        if (it != m_streams.end()) {
            // LOG_WARN("AudioMixer", "Stream already in mixer, not adding again");
            return;
        }
        
        m_streams.push_back(s);
        // LOG_INFO("AudioMixer", "Added stream, now have", m_streams.size(), "streams");
    }

    void audio_mixer::remove_stream(int token) {
        // Phase 3: Use write lock for modifications
        std::unique_lock lock(m_streams_mutex);
        
        auto it = std::remove_if(m_streams.begin(), m_streams.end(),
            [token](audio_stream* s) { return s->get_token() == token; });
        
        if (it != m_streams.end()) {
            m_streams.erase(it, m_streams.end());
            // LOG_INFO("AudioMixer", "Removed stream with token", token, ", now have", m_streams.size(), "streams");
        }
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
