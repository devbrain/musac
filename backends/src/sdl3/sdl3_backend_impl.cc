#include "sdl3_backend_impl.hh"
#include "sdl3_audio_stream.hh"
#include <musac/sdk/audio_stream_interface.hh>
#include <failsafe/failsafe.hh>

namespace musac {
    // Anonymous namespace for helper functions
    namespace {
        std::string get_sdl_error() {
            const char* error = SDL_GetError();
            return error ? error : "Unknown SDL error";
        }
#if defined(MUSAC_COMPILER_MSVC)
#pragma warning( push )
#pragma warning( disable : 4820)
#elif defined(MUSAC_COMPILER_GCC)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wold-style-cast"
# pragma GCC diagnostic ignored "-Wuseless-cast"
#elif defined(MUSAC_COMPILER_CLANG)
# pragma clang diagnostic push
#elif defined(MUSAC_COMPILER_WASM)
# pragma clang diagnostic push
#endif
        // Convert device ID to SDL3 device ID constant
        SDL_AudioDeviceID get_sdl_device_id(const std::string& device_id, bool playback) {
            if (device_id.empty() || device_id == "default") {
                return playback ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK : SDL_AUDIO_DEVICE_DEFAULT_RECORDING;
            }

            // Try to parse as numeric ID
            try {
                return static_cast <SDL_AudioDeviceID>(std::stoul(device_id));
            } catch (...) {
                // Not a numeric ID, return default
                return playback ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK : SDL_AUDIO_DEVICE_DEFAULT_RECORDING;
            }
        }
    }

#if defined(MUSAC_COMPILER_MSVC)
#pragma warning( pop )
#elif defined(MUSAC_COMPILER_GCC)
# pragma GCC diagnostic pop
#elif defined(MUSAC_COMPILER_CLANG)
# pragma clang diagnostic pop
#elif defined(MUSAC_COMPILER_WASM)
# pragma clang diagnostic pop
#endif

    // Format conversion helpers
    audio_format sdl3_backend::sdl_to_musac_format(SDL_AudioFormat sdl_fmt) {
        switch (sdl_fmt) {
            case SDL_AUDIO_U8: return audio_format::u8;
            case SDL_AUDIO_S8: return audio_format::s8;
            case SDL_AUDIO_S16LE: return audio_format::s16le;
            case SDL_AUDIO_S16BE: return audio_format::s16be;
            case SDL_AUDIO_S32LE: return audio_format::s32le;
            case SDL_AUDIO_S32BE: return audio_format::s32be;
            case SDL_AUDIO_F32LE: return audio_format::f32le;
            case SDL_AUDIO_F32BE: return audio_format::f32be;
            default: return audio_format::unknown;
        }
    }

    SDL_AudioFormat sdl3_backend::musac_to_sdl_format(audio_format fmt) {
        switch (fmt) {
            case audio_format::u8: return SDL_AUDIO_U8;
            case audio_format::s8: return SDL_AUDIO_S8;
            case audio_format::s16le: return SDL_AUDIO_S16LE;
            case audio_format::s16be: return SDL_AUDIO_S16BE;
            case audio_format::s32le: return SDL_AUDIO_S32LE;
            case audio_format::s32be: return SDL_AUDIO_S32BE;
            case audio_format::f32le: return SDL_AUDIO_F32LE;
            case audio_format::f32be: return SDL_AUDIO_F32BE;
            default: return SDL_AUDIO_S16LE; // Default fallback
        }
    }

    sdl3_backend::~sdl3_backend() {
        if (m_initialized) {
            shutdown();
        }
    }

    void sdl3_backend::init() {
        if (m_initialized) {
            THROW_RUNTIME("SDL3 backend already initialized");
        }

        // Initialize SDL audio subsystem
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            THROW_RUNTIME("Failed to initialize SDL3 audio: " + get_sdl_error());
        }

        m_initialized = true;
    }

    void sdl3_backend::shutdown() {
        if (!m_initialized) {
            return;
        }

        // Close all open devices
        {
            std::lock_guard <std::mutex> lock(m_devices_mutex);
            for (const auto& [handle, sdl_id] : m_open_devices) {
                SDL_CloseAudioDevice(sdl_id);
            }
            m_open_devices.clear();
            m_device_specs.clear();
        }

        // Quit SDL audio subsystem
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        m_initialized = false;
    }

    std::string sdl3_backend::get_name() const {
        return "SDL3";
    }

    bool sdl3_backend::is_initialized() const {
        return m_initialized;
    }

    std::vector <device_info_v2> sdl3_backend::enumerate_devices(bool playback) {
        if (!m_initialized) {
            THROW_RUNTIME("Backend not initialized");
        }

        std::vector <device_info_v2> devices;

        // Get device count
        int count = 0;
        SDL_AudioDeviceID* sdl_devices = SDL_GetAudioPlaybackDevices(&count);
        if (!playback) {
            sdl_devices = SDL_GetAudioRecordingDevices(&count);
        }

        if (sdl_devices) {
            for (size_t i = 0; i < static_cast <size_t>(count); i++) {
                const char* name = SDL_GetAudioDeviceName(sdl_devices[i]);
                if (!name) continue;

                // Get device format info
                SDL_AudioSpec spec;
                if (SDL_GetAudioDeviceFormat(sdl_devices[i], &spec, nullptr)) {
                    device_info_v2 info;
                    info.name = name;
                    info.id = std::to_string(sdl_devices[i]);
                    info.is_default = (i == 0); // SDL3 typically returns default first
                    info.channels = static_cast <channels_t>(spec.channels);
                    info.sample_rate = static_cast <sample_rate_t>(spec.freq);
                    devices.push_back(info);
                }
            }
            SDL_free(sdl_devices);
        }

        // If no devices found, add a default entry
        if (devices.empty()) {
            device_info_v2 info;
            info.name = playback ? "Default Playback" : "Default Recording";
            info.id = "default";
            info.is_default = true;
            info.channels = 2;
            info.sample_rate = 44100;
            devices.push_back(info);
        }

        return devices;
    }

    device_info_v2 sdl3_backend::get_default_device(bool playback) {
        auto devices = enumerate_devices(playback);
        if (!devices.empty()) {
            return devices[0]; // First device is typically default
        }

        // Return a fallback default
        device_info_v2 info;
        info.name = playback ? "Default Playback" : "Default Recording";
        info.id = "default";
        info.is_default = true;
        info.channels = 2;
        info.sample_rate = 44100;
        return info;
    }

    uint32_t sdl3_backend::open_device(const std::string& device_id,
                                       const audio_spec& spec,
                                       audio_spec& obtained_spec) {
        if (!m_initialized) {
            THROW_RUNTIME("Backend not initialized");
        }

        // Prepare SDL audio spec
        SDL_AudioSpec wanted;
        SDL_zero(wanted);
        wanted.freq = static_cast <int>(spec.freq);
        wanted.format = musac_to_sdl_format(spec.format);
        wanted.channels = spec.channels;

        // Open the device
        SDL_AudioDeviceID device_to_open = get_sdl_device_id(device_id, true);

        SDL_AudioDeviceID sdl_id = SDL_OpenAudioDevice(device_to_open, &wanted);
        if (sdl_id == 0) {
            THROW_RUNTIME("Failed to open audio device: " + get_sdl_error());
        }

        // Get the actual obtained spec
        SDL_AudioSpec obtained;
        if (!SDL_GetAudioDeviceFormat(sdl_id, &obtained, nullptr)) {
            SDL_CloseAudioDevice(sdl_id);
            THROW_RUNTIME("Failed to get audio device format: " + get_sdl_error());
        }

        // Fill obtained spec
        obtained_spec.freq = static_cast <uint32_t>(obtained.freq);
        obtained_spec.format = sdl_to_musac_format(obtained.format);
        obtained_spec.channels = static_cast <uint8_t>(obtained.channels);

        // Store device info
        uint32_t handle = 0; {
            std::lock_guard <std::mutex> lock(m_devices_mutex);
            handle = m_next_handle++;
            m_open_devices[handle] = sdl_id;
            m_device_specs[handle] = obtained_spec;
        }

        return handle;
    }

    void sdl3_backend::close_device(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_open_devices.find(device_handle);
        if (it != m_open_devices.end()) {
            SDL_CloseAudioDevice(it->second);
            m_open_devices.erase(it);
            m_device_specs.erase(device_handle);
        }
    }

    audio_format sdl3_backend::get_device_format(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_device_specs.find(device_handle);
        if (it == m_device_specs.end()) {
            THROW_RUNTIME("Invalid device handle");
        }
        return it->second.format;
    }

    sample_rate_t sdl3_backend::get_device_frequency(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_device_specs.find(device_handle);
        if (it == m_device_specs.end()) {
            THROW_RUNTIME("Invalid device handle");
        }
        return it->second.freq;
    }

    channels_t sdl3_backend::get_device_channels(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_device_specs.find(device_handle);
        if (it == m_device_specs.end()) {
            THROW_RUNTIME("Invalid device handle");
        }
        return it->second.channels;
    }

    float sdl3_backend::get_device_gain(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_open_devices.find(device_handle);
        if (it == m_open_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }

        float gain = SDL_GetAudioDeviceGain(it->second);
        return gain;
    }

    void sdl3_backend::set_device_gain(uint32_t device_handle, float gain) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_open_devices.find(device_handle);
        if (it == m_open_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }

        if (!SDL_SetAudioDeviceGain(it->second, gain)) {
            THROW_RUNTIME("Failed to set device gain: " + get_sdl_error());
        }
    }

    bool sdl3_backend::pause_device(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_open_devices.find(device_handle);
        if (it == m_open_devices.end()) {
            return false;
        }

        return SDL_PauseAudioDevice(it->second);
    }

    bool sdl3_backend::resume_device(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_open_devices.find(device_handle);
        if (it == m_open_devices.end()) {
            return false;
        }

        return SDL_ResumeAudioDevice(it->second);
    }

    bool sdl3_backend::is_device_paused(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_open_devices.find(device_handle);
        if (it == m_open_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }

        return SDL_AudioDevicePaused(it->second);
    }

    std::unique_ptr <audio_stream_interface> sdl3_backend::create_stream(
        uint32_t device_handle,
        const audio_spec& spec,
        void (* callback)(void* userdata, uint8_t* stream, int len),
        void* userdata) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_open_devices.find(device_handle);
        if (it == m_open_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }

        // Create SDL3 audio stream using the SDL device ID
        SDL_AudioDeviceID sdl_device = it->second;
        return std::make_unique <sdl3_audio_stream>(sdl_device, spec, callback, userdata);
    }

    bool sdl3_backend::supports_recording() const {
        return true;
    }

    int sdl3_backend::get_max_open_devices() const {
        // SDL3 doesn't have a hard limit, but let's be reasonable
        return 32;
    }

    SDL_AudioDeviceID sdl3_backend::get_sdl_device(uint32_t handle) const {
        std::lock_guard <std::mutex> lock(const_cast <std::mutex&>(m_devices_mutex));

        auto it = m_open_devices.find(handle);
        if (it == m_open_devices.end()) {
            return 0;
        }
        return it->second;
    }
} // namespace musac
