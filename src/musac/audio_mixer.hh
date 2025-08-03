//
// Created by igor on 4/28/25.
//

#pragma once

#include <memory>
#include <vector>
#include <shared_mutex>
#include <musac/sdk/buffer.hh>
#include <musac/audio_device_data.hh>
namespace musac {
    class audio_stream;

    class audio_mixer {
        public:
            audio_mixer();

            // Lock-free grab for the audio thread
            [[nodiscard]] std::shared_ptr <std::vector <audio_stream*>> get_streams() const;

            void add_stream(audio_stream* s);

            void remove_stream(int token);

            void resize(unsigned int out_len_samples);

            void set_zeros();

            void mix_channels(unsigned int channels,
                              unsigned int out_offset,
                              unsigned int cur_pos,
                              float volume_left,
                              float volume_right);
            /**
             * Access to the final mix buffer for conversion to device format.
            */

            float* finalMixData();

            [[nodiscard]] unsigned int allocatedSamples() const;


        private:
            // Phase 3: Proper thread-safe implementation using reader-writer lock
            mutable std::shared_mutex m_streams_mutex;
            std::vector<audio_stream*> m_streams;

        public:
            // Sample buffers we use during decoding and mixing.
            buffer <float> m_final_mix_buf;
            buffer <float> m_stream_buf;
            buffer <float> m_processor_buf;
            unsigned int m_allocated_samples = 0;
            static audio_device_data m_audio_device_data;
    };
}