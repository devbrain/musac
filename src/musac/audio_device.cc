//
// Created by igor on 3/17/25.
//

#include <musac/audio_device.hh>
#include <musac/sdk/samples_converter.hh>
#include <musac/sdk/from_float_converter.hh>
#include <musac/sdk/resampler.hh>


static std::vector<SDL_AudioDeviceID> s_devices {};

namespace musac {

    void close_audio_devices() {
        for (const auto device_id : s_devices) {
            SDL_CloseAudioDevice(device_id);
        }
    }

    audio_device::~audio_device() = default;

    audio_device::audio_device(audio_device&& other) noexcept
        : m_device_id(other.m_device_id),
          m_format(std::move(other.m_format)),
          m_channels(other.m_channels),
          m_freq(other.get_freq()),
          m_stream(other.m_stream) {
    }

    SDL_AudioDeviceID audio_device::get_device_id() const {
        return m_device_id;
    }

    SDL_AudioFormat audio_device::get_format() const {
        return m_format;
    }

    int audio_device::get_channels() const {
        return m_channels;
    }

    int audio_device::get_freq() const {
        return m_freq;
    }

    bool audio_device::pause() {
        return SDL_PauseAudioStreamDevice(m_stream.get());
    }

    bool audio_device::is_paused() const {
        return SDL_AudioStreamDevicePaused(m_stream.get());
    }

    bool audio_device::resume() {
        return SDL_ResumeAudioStreamDevice(m_stream.get());
    }

    float audio_device::get_gain() const {
        return SDL_GetAudioDeviceGain(m_device_id);
    }

    void audio_device::set_gain(float v) {
        SDL_SetAudioDeviceGain(m_device_id, v);
    }

    audio_stream audio_device::create_stream(audio_source&& audio_src) {
        return {std::move(audio_src)};
    }

    audio_device::audio_device(SDL_AudioDeviceID device_id, const SDL_AudioSpec* spec)
        : m_device_id(SDL_OpenAudioDevice(device_id, spec)),
          m_format(SDL_AUDIO_UNKNOWN),
          m_channels(0),
          m_freq(0) {
        SDL_AudioSpec audio_spec{};
        if (SDL_GetAudioDeviceFormat(device_id, &audio_spec, nullptr)) {
            m_channels = audio_spec.channels;
            m_format = audio_spec.format;
            m_freq = audio_spec.freq;
        }
        m_stream = std::shared_ptr<SDL_AudioStream>(
            SDL_OpenAudioDeviceStream(m_device_id, &audio_spec, sdl_audio_callback, this),
            SDL_DestroyAudioStream
            );
        audio_device_data aud;
        aud.m_audio_spec.freq = m_freq;
        aud.m_audio_spec.format = m_format;
        aud.m_audio_spec.channels = m_channels;
        aud.m_stream = m_stream;
        aud.m_frame_size = 4096;
        aud.m_sample_converter = get_from_float_converter(get_format());
        audio_stream::set_audio_device_data(aud);
    }

    void audio_device::sdl_audio_callback([[maybe_unused]] void* userdata, SDL_AudioStream* stream, int additional_amount,
                                          [[maybe_unused]] int total_amount) {

        if (additional_amount > 0) {
            auto data = SDL_stack_alloc(Uint8, (unsigned int)additional_amount);
            if (data) {
                audio_stream::sdl_audio_callback(data, (unsigned int)additional_amount);
                SDL_PutAudioStreamData(stream, data, additional_amount);
                SDL_stack_free(data);
            }
        }
    }

    std::vector <audio_hardware> audio_hardware::enumerate(bool playback_devices) {
        std::vector <audio_hardware> out;
        auto devid = SDL_OpenAudioDevice(
            playback_devices ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK : SDL_AUDIO_DEVICE_DEFAULT_RECORDING, nullptr);
        SDL_CloseAudioDevice(devid);

        int count = 0;
        SDL_AudioDeviceID* ids = playback_devices
                                     ? SDL_GetAudioPlaybackDevices(&count)
                                     : SDL_GetAudioRecordingDevices(&count);
        for (int i = 0; i < count; i++) {
            out.emplace_back(audio_hardware{ids[i], playback_devices, ids[i] == devid});
        }
        SDL_free(ids);
        return out;
    }

    audio_hardware audio_hardware::get_default_device(bool playback_device) {
        auto devid = SDL_OpenAudioDevice(
            playback_device ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK : SDL_AUDIO_DEVICE_DEFAULT_RECORDING, nullptr);

        audio_hardware out(devid, playback_device, true);
        SDL_CloseAudioDevice(devid);
        return out;
    }

    SDL_AudioDeviceID audio_hardware::get_device_id() const {
        return m_device_id;
    }

    bool audio_hardware::is_playback() const {
        return m_is_playback;
    }

    bool audio_hardware::is_default() const {
        return m_is_default;
    }

    SDL_AudioFormat audio_hardware::get_format() const {
        return m_format;
    }

    int audio_hardware::get_channels() const {
        return m_channels;
    }

    int audio_hardware::get_freq() const {
        return m_freq;
    }

    std::string audio_hardware::get_name() const {
        return m_device_name;
    }

    audio_device audio_hardware::open() const {
        SDL_AudioSpec spec{
            m_format,
            m_channels,
            m_freq
        };
        if (m_is_default) {
            return {
                m_is_playback ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK : SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
                &spec
            };
        }

        return {m_device_id, &spec};
    }

    audio_device audio_hardware::open(const SDL_AudioSpec& spec) const {
        if (m_is_default) {
            return {
                m_is_playback ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK : SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
                &spec
            };
        }
        return {m_device_id, &spec};
    }

    audio_hardware::audio_hardware(SDL_AudioDeviceID device_id, bool is_playback, bool is_default)
        : m_device_id(device_id),
          m_is_playback(is_playback),
          m_is_default(is_default),
          m_format(SDL_AUDIO_UNKNOWN),
          m_channels(0),
          m_freq(0) {
        SDL_AudioSpec audio_spec{};
        if (SDL_GetAudioDeviceFormat(device_id, &audio_spec, nullptr)) {
            m_channels = audio_spec.channels;
            m_format = audio_spec.format;
            m_freq = audio_spec.freq;
        }
        if (const auto* name = SDL_GetAudioDeviceName(m_device_id)) {
            m_device_name = name;
        }
    }
}
