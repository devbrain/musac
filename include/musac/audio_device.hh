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

namespace musac {
    // Forward declarations
    class audio_stream;
    class audio_source;
    class audio_hardware;

    class MUSAC_EXPORT audio_device {
        friend class audio_hardware;

        public:
            ~audio_device();
            audio_device(audio_device&&) noexcept;

            uint32_t get_device_id() const;
            audio_format get_format() const;
            int get_channels() const;
            int get_freq() const;

            bool pause();
            bool is_paused() const;
            bool resume();

            float get_gain() const;
            void set_gain(float v);

            audio_stream create_stream(audio_source&& audio_src);
            
            void create_stream_with_callback(
                void (*callback)(void* userdata, uint8_t* stream, int len),
                void* userdata);

        private:
            audio_device(const audio_hardware& hw, const audio_spec* spec);
            
        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };

    class MUSAC_EXPORT audio_hardware {
        public:
            static std::vector <audio_hardware> enumerate(bool playback_devices = true);
            static audio_hardware get_default_device(bool playback_device = true);

            std::string get_device_name() const;
            audio_format get_format() const;
            int get_channels() const;
            int get_freq() const;

            audio_device open_device(const audio_spec* spec = nullptr) const {
                return audio_device(*this, spec);
            }
            
            audio_hardware();
            ~audio_hardware();
            audio_hardware(audio_hardware&&) noexcept;
            audio_hardware& operator=(audio_hardware&&) noexcept;

        private:
            
            friend class audio_device;
            
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}

#endif