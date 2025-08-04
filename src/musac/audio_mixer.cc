//
// Created by igor on 4/28/25.
//

#include "musac/audio_mixer.hh"
#include <musac/stream.hh>
#include <failsafe/failsafe.hh>
#include <algorithm>
#include <SDL3/SDL.h>
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
    
    mixer_snapshot audio_mixer::capture_state() const {
        mixer_snapshot snapshot;
        
        // Capture timing
        snapshot.snapshot_time = std::chrono::steady_clock::now();
        snapshot.global_tick_count = SDL_GetTicks(); // TODO: Remove SDL dependency
        
        // Capture audio format
        snapshot.audio_spec.channels = m_audio_device_data.m_audio_spec.channels;
        snapshot.audio_spec.freq = m_audio_device_data.m_audio_spec.freq;
        snapshot.audio_spec.format = static_cast<int>(m_audio_device_data.m_audio_spec.format);
        
        // Capture any pending samples from the final mix buffer
        // These are samples that have been mixed but not yet consumed
        if (m_allocated_samples > 0) {
            snapshot.pending_samples.assign(
                m_final_mix_buf.begin(), 
                m_final_mix_buf.begin() + m_allocated_samples
            );
        }
        
        // Capture state of all active streams
        auto streams = get_streams();
        if (streams) {
            for (const auto& entry : *streams) {
                if (entry.stream && !entry.lifetime_token.expired()) {
                    mixer_snapshot::stream_state state;
                    auto* stream = entry.stream;
                    
                    // Capture stream identification
                    state.token = stream->get_token();
                    
                    // Capture stream state using the new API
                    auto stream_snap = stream->capture_state();
                    state.playback_tick = stream_snap.playback_tick;
                    state.current_frame = 0; // TODO: track current frame in audio_source
                    state.volume = stream_snap.volume;
                    state.internal_volume = stream_snap.internal_volume;
                    state.stereo_pos = stream_snap.stereo_pos;
                    state.is_playing = stream_snap.is_playing;
                    state.is_paused = stream_snap.is_paused;
                    state.is_muted = stream_snap.is_muted;
                    state.starting = stream_snap.starting;
                    state.current_iteration = stream_snap.current_iteration;
                    state.wanted_iterations = stream_snap.wanted_iterations;
                    state.fade_gain = stream_snap.fade_gain;
                    state.fade_state = stream_snap.fade_state;
                    state.playback_start_tick = stream_snap.playback_start_tick;
                    
                    snapshot.active_streams.push_back(state);
                }
            }
        }
        
        return snapshot;
    }
    
    void audio_mixer::restore_state(const mixer_snapshot& snapshot) {
        // Restore audio format
        m_audio_device_data.m_audio_spec.channels = snapshot.audio_spec.channels;
        m_audio_device_data.m_audio_spec.freq = snapshot.audio_spec.freq;
        m_audio_device_data.m_audio_spec.format = static_cast<audio_format>(snapshot.audio_spec.format);
        
        // Restore pending samples to the mix buffer
        if (!snapshot.pending_samples.empty()) {
            resize(static_cast<unsigned int>(snapshot.pending_samples.size()));
            std::copy(snapshot.pending_samples.begin(), 
                     snapshot.pending_samples.end(),
                     m_final_mix_buf.begin());
        }
        
        // TODO: Restore stream states once we have friend access
    }
}
