//
// Created by igor on 3/17/25.
//

#ifndef  AUDIO_DEVICE_HH
#define  AUDIO_DEVICE_HH

#include <vector>
#include <string>
#include <memory>
#include <musac/export_musac.h>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/types.hh>

namespace musac {
    // Device information structure (from v1, kept for compatibility)
    struct device_info {
        std::string name;
        std::string id;
        bool is_default;
        channels_t channels;
        sample_rate_t sample_rate;
    };
    
    // Forward declarations
    class audio_stream;
    class pc_speaker_stream;
    class audio_source;
    class audio_backend;
    struct device_info_v2;

    class MUSAC_EXPORT audio_device {
        public:
            // New factory methods that accept backend (v2 API)
            static std::vector<device_info> enumerate_devices(
                std::shared_ptr<audio_backend> backend,
                bool playback_devices = true);
            static audio_device open_default_device(
                std::shared_ptr<audio_backend> backend,
                const audio_spec* spec = nullptr);
            static audio_device open_device(
                std::shared_ptr<audio_backend> backend,
                const std::string& device_id, 
                const audio_spec* spec = nullptr);
            

            ~audio_device();
            audio_device(audio_device&&) noexcept;

            // Device properties
            std::string get_device_name() const;
            std::string get_device_id() const;
            audio_format get_format() const;
            channels_t get_channels() const;
            sample_rate_t get_freq() const;

            // Playback control
            bool pause();
            bool is_paused() const;
            bool resume();

            float get_gain() const;
            void set_gain(float v);

            // Stream creation
            audio_stream create_stream(audio_source&& audio_src);
            pc_speaker_stream create_pc_speaker_stream();
            
            void create_stream_with_callback(
                void (*callback)(void* userdata, uint8_t* stream, int len),
                void* userdata);
            
            // Get final mixed output buffer for visualization
            std::vector<float> get_output_buffer() const;

        private:
            // Constructor with backend
            audio_device(
                std::shared_ptr<audio_backend> backend,
                const device_info_v2& info, 
                const audio_spec* spec);
            
            // For device switching
            friend class audio_system;
            
        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}

#endif