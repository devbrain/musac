//
// Created by igor on 3/17/25.
//

#ifndef  AUDIO_DEVICE_HH
#define  AUDIO_DEVICE_HH

#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <musac/stream.hh>
#include <musac/audio_source.hh>

namespace musac {
    class audio_hardware;

    class audio_device {
        friend class audio_hardware;

        public:
            ~audio_device();
            audio_device(audio_device&&) noexcept;

            SDL_AudioDeviceID get_device_id() const;
            SDL_AudioFormat get_format() const;
            int get_channels() const;
            int get_freq() const;

            bool pause();
            bool is_paused() const;
            bool resume();

            float get_gain() const;
            void set_gain(float v);

            audio_stream create_stream(audio_source&& audio_src);

        private:
            audio_device(SDL_AudioDeviceID device_id, const SDL_AudioSpec* spec);
            static void SDLCALL sdl_audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount,
                                                   int total_amount);
        private:
            SDL_AudioDeviceID m_device_id;
            SDL_AudioFormat m_format;
            int m_channels;
            int m_freq;
            std::shared_ptr<SDL_AudioStream> m_stream;
    };

    class audio_hardware {
        public:
            static std::vector <audio_hardware> enumerate(bool playback_devices = true);
            static audio_hardware get_default_device(bool playback_device = true);

            SDL_AudioDeviceID get_device_id() const;
            bool is_playback() const;
            bool is_default() const;
            SDL_AudioFormat get_format() const;
            int get_channels() const;
            int get_freq() const;
            std::string get_name() const;

            audio_device open() const;
            audio_device open(const SDL_AudioSpec& spec) const;

        private:
            audio_hardware(SDL_AudioDeviceID device_id, bool is_playback, bool is_default);

        private:
            SDL_AudioDeviceID m_device_id;
            bool m_is_playback;
            bool m_is_default;
            SDL_AudioFormat m_format;
            int m_channels;
            int m_freq;
            std::string m_device_name;
    };
}

#endif
