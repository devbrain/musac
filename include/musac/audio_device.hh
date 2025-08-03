//
// Created by igor on 3/17/25.
//

#ifndef  AUDIO_DEVICE_HH
#define  AUDIO_DEVICE_HH

#include <vector>
#include <string>
#include <memory>
#include <musac/export_musac.h>
#include <musac/sdk/audio_format.h>
#include <musac/audio_device_interface.hh>

namespace musac {
    // Forward declarations
    class audio_stream;
    class audio_source;

    class MUSAC_EXPORT audio_device {
        public:
            // Factory methods
            static std::vector<device_info> enumerate_devices(bool playback_devices = true);
            static audio_device open_default_device(const audio_spec* spec = nullptr);
            static audio_device open_device(const std::string& device_id, const audio_spec* spec = nullptr);

            ~audio_device();
            audio_device(audio_device&&) noexcept;

            // Device properties
            std::string get_device_name() const;
            std::string get_device_id() const;
            audio_format get_format() const;
            int get_channels() const;
            int get_freq() const;

            // Playback control
            bool pause();
            bool is_paused() const;
            bool resume();

            float get_gain() const;
            void set_gain(float v);

            // Stream creation
            audio_stream create_stream(audio_source&& audio_src);
            
            void create_stream_with_callback(
                void (*callback)(void* userdata, uint8_t* stream, int len),
                void* userdata);

        private:
            explicit audio_device(const device_info& info, const audio_spec* spec);
            
        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}

#endif