#include "sdl3_backend_impl.hh"
#include "sdl3_audio_stream.hh"
#include <musac/sdk/audio_stream_interface.hh>
#include <failsafe/failsafe.hh>
#include <algorithm>

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
            for (const auto& [handle, info] : m_devices) {
                SDL_CloseAudioDevice(info.sdl_id);
            }
            m_devices.clear();
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

    std::vector <device_info> sdl3_backend::enumerate_devices(bool playback) {
        if (!m_initialized) {
            THROW_RUNTIME("Backend not initialized");
        }

        std::vector <device_info> devices;

        // Get device count
        int count = 0;
        SDL_AudioDeviceID* sdl_devices = SDL_GetAudioPlaybackDevices(&count);
        if (!playback) {
            sdl_devices = SDL_GetAudioRecordingDevices(&count);
        }

        // To find the actual default device, open the default device and get its name
        std::string default_device_name;
        SDL_AudioSpec dummy_spec;
        SDL_zero(dummy_spec);
        dummy_spec.freq = 44100;
        dummy_spec.format = SDL_AUDIO_F32LE;
        dummy_spec.channels = 2;
        
#if defined(MUSAC_COMPILER_GCC)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wold-style-cast"
# pragma GCC diagnostic ignored "-Wuseless-cast"
#elif defined(MUSAC_COMPILER_CLANG)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wold-style-cast"
#endif
        SDL_AudioDeviceID test_default = SDL_OpenAudioDevice(
            playback ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK : SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
            &dummy_spec
        );
#if defined(MUSAC_COMPILER_GCC)
# pragma GCC diagnostic pop
#elif defined(MUSAC_COMPILER_CLANG)
# pragma clang diagnostic pop
#endif
        
        if (test_default != 0) {
            // Get the name of the device that was actually opened
            const char* opened_name = SDL_GetAudioDeviceName(test_default);
            if (opened_name) {
                default_device_name = opened_name;
            }
            SDL_CloseAudioDevice(test_default);
        }

        if (sdl_devices) {
            // First, collect all devices
            for (size_t i = 0; i < static_cast <size_t>(count); i++) {
                const char* name = SDL_GetAudioDeviceName(sdl_devices[i]);
                if (!name) continue;

                // Get device format info
                SDL_AudioSpec spec;
                if (SDL_GetAudioDeviceFormat(sdl_devices[i], &spec, nullptr)) {
                    device_info info;
                    info.name = name;
                    info.id = std::to_string(sdl_devices[i]);
                    // Check if this device name matches the default device name
                    info.is_default = (!default_device_name.empty() && info.name == default_device_name);
                    info.channels = static_cast <channels_t>(spec.channels);
                    info.sample_rate = static_cast <sample_rate_t>(spec.freq);
                    devices.push_back(info);
                }
            }
            SDL_free(sdl_devices);
        }

        // If we found a default device, move it to the front
        if (!default_device_name.empty()) {
            auto default_it = std::find_if(devices.begin(), devices.end(),
                [](const device_info& dev) { return dev.is_default; });
            
            if (default_it != devices.end() && default_it != devices.begin()) {
                // Move the default device to the front
                device_info default_device = *default_it;
                devices.erase(default_it);
                devices.insert(devices.begin(), default_device);
            }
        }
        
        // If no device was marked as default (name didn't match), mark the first one
        if (!devices.empty() && std::none_of(devices.begin(), devices.end(),
            [](const device_info& dev) { return dev.is_default; })) {
            devices[0].is_default = true;
        }

        // If no devices found, add a default entry
        if (devices.empty()) {
            device_info info;
            info.name = playback ? "Default Playback" : "Default Recording";
            info.id = "default";
            info.is_default = true;
            info.channels = 2;
            info.sample_rate = 44100;
            devices.push_back(info);
        }

        return devices;
    }

    device_info sdl3_backend::get_default_device(bool playback) {
        auto devices = enumerate_devices(playback);
        if (!devices.empty()) {
            return devices[0]; // First device is typically default
        }

        // Return a fallback default
        device_info info;
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
            
            // Create and populate device state
            device_state& info = m_devices[handle];
            info.sdl_id = sdl_id;
            info.spec = obtained_spec;
            info.gain = 1.0f;  // Default gain
            info.is_muted = false;  // Default not muted
        }

        return handle;
    }

    void sdl3_backend::close_device(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            SDL_CloseAudioDevice(it->second.sdl_id);
            m_devices.erase(it);
        }
        // Silently ignore invalid handles - this matches original behavior
        // and prevents crashes during cleanup
    }

    audio_format sdl3_backend::get_device_format(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }
        return it->second.spec.format;
    }

    sample_rate_t sdl3_backend::get_device_frequency(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }
        return it->second.spec.freq;
    }

    channels_t sdl3_backend::get_device_channels(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }
        return it->second.spec.channels;
    }

    float sdl3_backend::get_device_gain(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }

        float gain = SDL_GetAudioDeviceGain(it->second.sdl_id);
        return gain;
    }

    void sdl3_backend::set_device_gain(uint32_t device_handle, float gain) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }

        if (!SDL_SetAudioDeviceGain(it->second.sdl_id, gain)) {
            THROW_RUNTIME("Failed to set device gain: " + get_sdl_error());
        }
    }

    bool sdl3_backend::pause_device(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            return false;
        }

        return SDL_PauseAudioDevice(it->second.sdl_id);
    }

    bool sdl3_backend::resume_device(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            return false;
        }

        return SDL_ResumeAudioDevice(it->second.sdl_id);
    }

    bool sdl3_backend::is_device_paused(uint32_t device_handle) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }

        return SDL_AudioDevicePaused(it->second.sdl_id);
    }

    bool sdl3_backend::supports_mute() const {
        return true; // SDL3 supports pause which we use for muting
    }

    bool sdl3_backend::mute_device(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            // Track mute state
            it->second.is_muted = true;
            // Use SDL pause to implement mute
            return SDL_PauseAudioDevice(it->second.sdl_id);
        }
        return false;
    }

    bool sdl3_backend::unmute_device(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            // Clear mute state
            it->second.is_muted = false;
            // Resume audio playback
            return SDL_ResumeAudioDevice(it->second.sdl_id);
        }
        return false;
    }

    bool sdl3_backend::is_device_muted(uint32_t device_handle) const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_devices_mutex));
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            return it->second.is_muted;
        }
        return false;
    }

    std::unique_ptr <audio_stream_interface> sdl3_backend::create_stream(
        uint32_t device_handle,
        const audio_spec& spec,
        void (* callback)(void* userdata, uint8_t* stream, int len),
        void* userdata) {
        std::lock_guard <std::mutex> lock(m_devices_mutex);

        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }

        // Create SDL3 audio stream using the SDL device ID
        SDL_AudioDeviceID sdl_device = it->second.sdl_id;
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

        auto it = m_devices.find(handle);
        if (it == m_devices.end()) {
            return 0;
        }
        return it->second.sdl_id;
    }
} // namespace musac
