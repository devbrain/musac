//
// Created by igor on 4/28/25.
//

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

    class MUSAC_EXPORT audio_mixer {
        public:
            audio_mixer();
            
            // Delete copy operations (contains static data and buffers)
            audio_mixer(const audio_mixer&) = delete;
            audio_mixer& operator=(const audio_mixer&) = delete;
            
            // Default move operations
            audio_mixer(audio_mixer&&) = default;
            audio_mixer& operator=(audio_mixer&&) = default;

            // Get valid streams for the audio thread
            [[nodiscard]] std::shared_ptr<std::vector<stream_container::stream_entry>> get_streams() const;

            void add_stream(audio_stream* s, std::weak_ptr<void> lifetime_token);

            void remove_stream(int token);
            
            void update_stream_pointer(int token, audio_stream* new_stream);

            void resize(size_t out_len_samples);

            void set_zeros();

            void mix_channels(channels_t channels,
                              size_t out_offset,
                              size_t cur_pos,
                              float volume_left,
                              float volume_right);
            /**
             * Access to the final mix buffer for conversion to device format.
            */

            float* final_mix_data();

            [[nodiscard]] std::size_t allocated_samples() const;
            
            // Buffer management - force compaction during quiet periods
            void compact_buffers();
            
            // Device switching support
            [[nodiscard]] mixer_snapshot capture_state() const;
            void restore_state(const mixer_snapshot& snapshot);
            
            // Final output buffer access for visualization
            void capture_final_output(const float* buffer, size_t samples);
            [[nodiscard]] std::vector<float> get_final_output() const;

        private:
            // Ring buffer for final output (for visualization)
            static constexpr size_t OUTPUT_BUFFER_SIZE = 8192;
            mutable std::vector<float> m_final_output_buffer;
            mutable std::atomic<size_t> m_output_write_pos{0};
            
        private:
            // Encapsulated stream management
            stream_container m_stream_container;

        public:
            // Sample buffers we use during decoding and mixing.
            buffer <float> m_final_mix_buf;
            buffer <float> m_stream_buf;
            buffer <float> m_processor_buf;
            std::size_t m_allocated_samples = 0;
            static audio_device_data m_audio_device_data;
    };
}