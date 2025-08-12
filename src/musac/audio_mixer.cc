//
// Created by igor on 4/28/25.
//

#include "musac/audio_mixer.hh"
#include <musac/stream.hh>
#include <failsafe/failsafe.hh>
#include <algorithm>
#include <chrono>
namespace musac {
    audio_device_data audio_mixer::m_audio_device_data {};

    audio_mixer::audio_mixer(): m_final_mix_buf(0),
                                m_stream_buf(0),
                                m_processor_buf(0),
                                m_final_output_buffer(OUTPUT_BUFFER_SIZE, 0.0f) {
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
    
    void audio_mixer::update_stream_pointer(int token, audio_stream* new_stream) {
        m_stream_container.update_stream_pointer(token, new_stream);
    }

    void audio_mixer::resize(size_t out_len_samples) {
        // Buffer management constants
        static constexpr size_t MAX_RETAINED_SAMPLES = 256 * 1024;  // ~1MB for float (256K * 4 bytes)
        static constexpr size_t MIN_BUFFER_SAMPLES = 4096;          // Minimum buffer size
        static constexpr float SHRINK_THRESHOLD = 0.25f;            // Shrink if using < 25%
        static constexpr size_t SHRINK_HEADROOM = 2;                // Shrink to 2x current need
        static constexpr size_t STABILITY_FRAMES = 100;             // Wait 100 callbacks before shrinking
        
        // Track usage patterns for stability
        static size_t frames_at_current_size = 0;
        static size_t consecutive_small_requests = 0;
        
        if (out_len_samples > m_allocated_samples) {
            // Need to grow - do it immediately for real-time safety
            m_final_mix_buf.resize(out_len_samples);
            m_stream_buf.resize(out_len_samples);
            m_processor_buf.resize(out_len_samples);
            m_allocated_samples = out_len_samples;
            
            // Reset stability counters since size changed
            frames_at_current_size = 0;
            consecutive_small_requests = 0;
            
            // Log significant growth for debugging
            if (out_len_samples > MAX_RETAINED_SAMPLES) {
                LOG_WARN("audio_mixer", "Large buffer allocation:", out_len_samples, "samples");
            }
            
        } else if (m_allocated_samples > MAX_RETAINED_SAMPLES && 
                   out_len_samples < static_cast<unsigned int>(m_allocated_samples * SHRINK_THRESHOLD)) {
            // Buffer is large and we're using less than 25% of it
            consecutive_small_requests++;
            
            // Only shrink after consistent small usage for STABILITY_FRAMES callbacks
            if (consecutive_small_requests > STABILITY_FRAMES) {
                // Calculate new size with headroom for growth
                size_t new_size = std::max(
                    static_cast<size_t>(out_len_samples * SHRINK_HEADROOM),
                    MIN_BUFFER_SAMPLES
                );
                
                // Ensure we don't grow beyond MAX_RETAINED_SAMPLES
                new_size = std::min(new_size, MAX_RETAINED_SAMPLES);
                
                // Resize buffers (but don't shrink_to_fit to avoid reallocation in audio thread)
                m_final_mix_buf.resize(new_size);
                m_stream_buf.resize(new_size);
                m_processor_buf.resize(new_size);
                m_allocated_samples = static_cast<unsigned int>(new_size);
                
                // Reset counters
                consecutive_small_requests = 0;
                frames_at_current_size = 0;
                
                LOG_INFO("audio_mixer", "Shrunk buffers from", m_allocated_samples, "to", new_size, "samples");
            }
        } else {
            // Size is acceptable, just track stability
            frames_at_current_size++;
            
            // Reset small request counter if we're using a reasonable amount
            if (out_len_samples >= static_cast<unsigned int>(m_allocated_samples * SHRINK_THRESHOLD)) {
                consecutive_small_requests = 0;
            }
        }
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


    void audio_mixer::mix_channels(channels_t channels, size_t out_offset, size_t cur_pos,
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

    float* audio_mixer::final_mix_data() {
        return m_final_mix_buf.data();
    }

    std::size_t audio_mixer::allocated_samples() const {
        return m_allocated_samples;
    }
    
    void audio_mixer::compact_buffers() {
        // This can be called by the application during quiet periods
        // (e.g., menu screens, level transitions) to force memory cleanup
        
        static constexpr size_t MIN_BUFFER_SAMPLES = 4096;
        
        // Only compact if we have excessively large buffers
        if (m_allocated_samples > MIN_BUFFER_SAMPLES * 4) {
            auto old_size = m_allocated_samples;
            
            // Resize buffers - buffer::resize() already reallocates memory
            // so we don't need shrink_to_fit()
            m_final_mix_buf.resize(MIN_BUFFER_SAMPLES);
            m_stream_buf.resize(MIN_BUFFER_SAMPLES);
            m_processor_buf.resize(MIN_BUFFER_SAMPLES);
            
            m_allocated_samples = MIN_BUFFER_SAMPLES;
            
            LOG_INFO("audio_mixer", "Compacted buffers from", old_size, "to", MIN_BUFFER_SAMPLES, "samples");
        }
    }
    
    mixer_snapshot audio_mixer::capture_state() const {
        mixer_snapshot snapshot;
        
        // Capture timing
        snapshot.snapshot_time = std::chrono::steady_clock::now();
        // Calculate milliseconds since some fixed point (similar to SDL_GetTicks)
        static auto start_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        snapshot.global_tick_count = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
        );
        
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
    
    void audio_mixer::capture_final_output(const float* buffer, size_t samples) {
        if (!buffer || samples == 0) return;
        
        size_t pos = m_output_write_pos.load(std::memory_order_relaxed);
        
        // Copy samples to ring buffer
        for (size_t i = 0; i < samples; ++i) {
            m_final_output_buffer[(pos + i) % OUTPUT_BUFFER_SIZE] = buffer[i];
        }
        
        // Update write position
        m_output_write_pos.store((pos + samples) % OUTPUT_BUFFER_SIZE, std::memory_order_relaxed);
    }
    
    std::vector<float> audio_mixer::get_final_output() const {
        std::vector<float> result(OUTPUT_BUFFER_SIZE);
        size_t write_pos = m_output_write_pos.load(std::memory_order_relaxed);
        
        // Copy from ring buffer starting from write position (oldest data)
        for (size_t i = 0; i < OUTPUT_BUFFER_SIZE; ++i) {
            result[i] = m_final_output_buffer[(write_pos + i) % OUTPUT_BUFFER_SIZE];
        }
        
        return result;
    }
    
    void audio_mixer::mute_all() {
        m_global_muted.store(true, std::memory_order_relaxed);
    }
    
    void audio_mixer::unmute_all() {
        m_global_muted.store(false, std::memory_order_relaxed);
    }
    
    bool audio_mixer::is_all_muted() const {
        return m_global_muted.load(std::memory_order_relaxed);
    }
}
