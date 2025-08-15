/**
 * @file audio_mixer.hh
 * @brief Internal audio mixing engine
 * @ingroup internal
 */

#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <musac/sdk/buffer.hh>
#include <musac/sdk/types.hh>
#include <musac/audio_device_data.hh>
#include <musac/export_musac.h>
#include "mixer_snapshot.hh"
#include "stream_container.hh"

namespace musac {
    class audio_stream;

    /**
     * @class audio_mixer
     * @brief Core mixing engine for combining multiple audio streams
     * @ingroup internal
     * 
     * The audio_mixer is the heart of the audio system, responsible for
     * combining multiple audio streams into a single output buffer that
     * gets sent to the audio device. It runs in the audio callback thread
     * with real-time constraints.
     * 
     * ## Architecture
     * 
     * The mixer uses a lock-free design with weak pointer lifetime management
     * to ensure thread safety without blocking the audio thread:
     * 
     * - **Stream Management**: Uses weak_ptr tokens to safely access streams
     * - **Lock-Free Mixing**: Audio thread never blocks on mutex operations
     * - **Buffer Pooling**: Pre-allocated buffers to avoid memory allocation
     * - **Device Switching**: Snapshot/restore for seamless device changes
     * 
     * ## Thread Safety
     * 
     * - Audio thread (callback) reads stream data without locking
     * - Control thread adds/removes streams with minimal locking
     * - Weak pointers prevent use-after-free in concurrent access
     * 
     * ## Performance Characteristics
     * 
     * - Zero allocations in audio callback
     * - Cache-friendly buffer layout
     * - SIMD-optimized mixing operations (platform-dependent)
     * - Automatic buffer compaction during quiet periods
     * 
     * @warning This is an internal class not intended for direct use
     * @see audio_device, audio_stream, stream_container
     */
    class MUSAC_EXPORT audio_mixer {
        public:
            /**
             * @brief Constructor
             */
            audio_mixer();
            
            // Delete copy operations (contains static data and buffers)
            audio_mixer(const audio_mixer&) = delete;
            audio_mixer& operator=(const audio_mixer&) = delete;
            
            // Default move operations
            audio_mixer(audio_mixer&&) = default;
            audio_mixer& operator=(audio_mixer&&) = default;

            /**
             * @brief Get valid streams for mixing
             * @return Shared pointer to stream entries with valid lifetime tokens
             * 
             * Called from audio thread. Returns only streams whose lifetime tokens
             * are still valid, ensuring safe access without locking.
             * 
             * @note Real-time safe - no allocations or locks
             */
            [[nodiscard]] std::shared_ptr<std::vector<stream_container::stream_entry>> get_streams() const;

            /**
             * @brief Add a new stream to the mixer
             * @param s Pointer to the audio stream
             * @param lifetime_token Weak pointer for lifetime management
             * 
             * The lifetime token ensures the stream can be safely accessed from
             * the audio thread even if it's being destroyed on another thread.
             */
            void add_stream(audio_stream* s, std::weak_ptr<void> lifetime_token);

            /**
             * @brief Remove a stream from the mixer
             * @param token Stream identifier token
             */
            void remove_stream(int token);
            
            /**
             * @brief Update stream pointer (for move operations)
             * @param token Stream identifier token
             * @param new_stream New stream pointer after move
             */
            void update_stream_pointer(int token, audio_stream* new_stream);

            /**
             * @brief Resize internal buffers
             * @param out_len_samples New buffer size in samples
             * 
             * Adjusts buffer sizes to match device requirements.
             * Called when device parameters change.
             */
            void resize(size_t out_len_samples);

            /**
             * @brief Clear the mix buffer to silence
             * 
             * Called at the start of each audio callback to prepare
             * for mixing.
             */
            void set_zeros();

            /**
             * @brief Mix stream data into final buffer
             * @param channels Number of channels in stream
             * @param out_offset Offset in output buffer
             * @param cur_pos Current position in stream buffer
             * @param volume_left Left channel volume multiplier
             * @param volume_right Right channel volume multiplier
             * 
             * Core mixing function that combines stream audio with the
             * final mix buffer, applying volume and pan.
             */
            void mix_channels(channels_t channels,
                              size_t out_offset,
                              size_t cur_pos,
                              float volume_left,
                              float volume_right);
            
            /**
             * @brief Access the final mix buffer
             * @return Pointer to mix buffer data
             * 
             * Used by audio callback to get mixed audio for output.
             */
            float* final_mix_data();

            /**
             * @brief Get allocated buffer size
             * @return Number of samples allocated
             */
            [[nodiscard]] std::size_t allocated_samples() const;
            
            /**
             * @brief Compact buffers to reduce memory usage
             * 
             * Called during quiet periods to free unused memory.
             */
            void compact_buffers();
            
            /**
             * @brief Capture mixer state for device switching
             * @return Snapshot of current mixer state
             * 
             * Captures stream positions and states for seamless
             * device switching.
             */
            [[nodiscard]] mixer_snapshot capture_state() const;
            
            /**
             * @brief Restore mixer state after device switch
             * @param snapshot Previously captured state
             */
            void restore_state(const mixer_snapshot& snapshot);
            
            /**
             * @brief Capture output for visualization
             * @param buffer Audio buffer to capture
             * @param samples Number of samples
             * 
             * Stores recent audio output for visualizers.
             */
            void capture_final_output(const float* buffer, size_t samples);
            
            /**
             * @brief Get captured output for visualization
             * @return Recent audio samples
             */
            [[nodiscard]] std::vector<float> get_final_output() const;
            
            /**
             * @brief Mute all streams (mixer-level)
             * 
             * Fallback mute when backend doesn't support device muting.
             * Sets output to silence without stopping streams.
             */
            void mute_all();
            
            /**
             * @brief Unmute all streams
             */
            void unmute_all();
            
            /**
             * @brief Check if mixer is muted
             * @return true if globally muted
             */
            [[nodiscard]] bool is_all_muted() const;

        private:
            /**
             * @brief Ring buffer size for visualization
             */
            static constexpr size_t OUTPUT_BUFFER_SIZE = 8192;
            
            /**
             * @brief Ring buffer for final output (for visualization)
             */
            mutable std::vector<float> m_final_output_buffer;
            
            /**
             * @brief Write position in ring buffer
             */
            mutable std::atomic<size_t> m_output_write_pos{0};
            
            /**
             * @brief Global mute state for mixer-level fallback
             */
            std::atomic<bool> m_global_muted{false};
            
        private:
            /**
             * @brief Thread-safe stream container
             * 
             * Manages stream lifetime with weak pointers for lock-free access
             */
            stream_container m_stream_container;

        public:
            /**
             * @brief Final mix buffer
             * 
             * Contains the mixed audio from all streams, ready for output
             */
            buffer <float> m_final_mix_buf;
            
            /**
             * @brief Temporary buffer for stream decoding
             * 
             * Used to hold decoded samples from individual streams
             */
            buffer <float> m_stream_buf;
            
            /**
             * @brief Buffer for audio processors
             * 
             * Used when streams have effects or processors attached
             */
            buffer <float> m_processor_buf;
            
            /**
             * @brief Current allocated buffer size in samples
             */
            std::size_t m_allocated_samples = 0;
            
            /**
             * @brief Shared device data
             * 
             * Contains device parameters shared across the audio system
             */
            static audio_device_data m_audio_device_data;
    };
}