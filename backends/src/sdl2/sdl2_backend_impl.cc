#include "sdl2_backend_impl.hh"
#include "sdl2_audio_stream.hh"
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
    }

    // Format conversion helpers
    audio_format sdl2_backend::sdl_to_musac_format(SDL_AudioFormat sdl_fmt) {
        switch (sdl_fmt) {
            case AUDIO_U8: return audio_format::u8;
            case AUDIO_S8: return audio_format::s8;
            case AUDIO_S16LSB: return audio_format::s16le;
            case AUDIO_S16MSB: return audio_format::s16be;
            case AUDIO_S32LSB: return audio_format::s32le;
            case AUDIO_S32MSB: return audio_format::s32be;
            case AUDIO_F32LSB: return audio_format::f32le;
            case AUDIO_F32MSB: return audio_format::f32be;
            default: return audio_format::unknown;
        }
    }

    SDL_AudioFormat sdl2_backend::musac_to_sdl_format(audio_format fmt) {
        switch (fmt) {
            case audio_format::u8: return AUDIO_U8;
            case audio_format::s8: return AUDIO_S8;
            case audio_format::s16le: return AUDIO_S16LSB;
            case audio_format::s16be: return AUDIO_S16MSB;
            case audio_format::s32le: return AUDIO_S32LSB;
            case audio_format::s32be: return AUDIO_S32MSB;
            case audio_format::f32le: return AUDIO_F32LSB;
            case audio_format::f32be: return AUDIO_F32MSB;
            default: return AUDIO_S16LSB; // Default fallback
        }
    }

    sdl2_backend::~sdl2_backend() {
        if (m_initialized) {
            shutdown();
        }
    }

    void sdl2_backend::init() {
        if (m_initialized) {
            THROW_RUNTIME("SDL2 backend already initialized");
        }

        // Initialize SDL audio subsystem
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            THROW_RUNTIME("Failed to initialize SDL2 audio: " + get_sdl_error());
        }

        m_initialized = true;
    }

    void sdl2_backend::shutdown() {
        if (!m_initialized) {
            return;
        }

        // Close all open devices
        {
            std::lock_guard<std::mutex> lock(m_devices_mutex);
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

    std::string sdl2_backend::get_name() const {
        return "SDL2";
    }

    bool sdl2_backend::is_initialized() const {
        return m_initialized;
    }

    std::vector<device_info_v2> sdl2_backend::enumerate_devices(bool playback) {
        if (!m_initialized) {
            THROW_RUNTIME("Backend not initialized");
        }

        std::vector<device_info_v2> devices;

        // Get device count
        int count = SDL_GetNumAudioDevices(playback ? 0 : 1);
        
        for (int i = 0; i < count; i++) {
            const char* name = SDL_GetAudioDeviceName(i, playback ? 0 : 1);
            if (!name) continue;

            device_info_v2 info;
            info.name = name;
            info.id = std::to_string(i);
            info.is_default = (i == 0); // SDL2 typically returns default first
            // SDL2 doesn't provide device specs without opening, use defaults
            info.channels = 2;
            info.sample_rate = 44100;
            devices.push_back(info);
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

    device_info_v2 sdl2_backend::get_default_device(bool playback) {
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

    uint32_t sdl2_backend::open_device(const std::string& device_id,
                                       const audio_spec& spec,
                                       audio_spec& obtained_spec) {
        if (!m_initialized) {
            THROW_RUNTIME("Backend not initialized");
        }

        // Prepare SDL audio spec
        SDL_AudioSpec wanted;
        SDL_zero(wanted);
        wanted.freq = static_cast<int>(spec.freq);
        wanted.format = musac_to_sdl_format(spec.format);
        wanted.channels = spec.channels;
        wanted.samples = 4096; // SDL2 default buffer size
        wanted.callback = nullptr; // We'll use SDL_QueueAudio
        wanted.userdata = nullptr;

        SDL_AudioSpec obtained;
        
        // Parse device name/index
        const char* device_name = nullptr;
        if (!device_id.empty() && device_id != "default") {
            // In SDL2, we can pass device name directly
            device_name = device_id.c_str();
        }

        SDL_AudioDeviceID sdl_device = SDL_OpenAudioDevice(
            device_name,
            0, // playback
            &wanted,
            &obtained,
            SDL_AUDIO_ALLOW_FORMAT_CHANGE | SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE
        );

        if (sdl_device == 0) {
            THROW_RUNTIME("Failed to open audio device: " + get_sdl_error());
        }

        // Fill obtained spec
        obtained_spec.freq = static_cast<uint32_t>(obtained.freq);
        obtained_spec.format = sdl_to_musac_format(obtained.format);
        obtained_spec.channels = obtained.channels;

        // Store device info
        uint32_t handle;
        {
            std::lock_guard<std::mutex> lock(m_devices_mutex);
            handle = m_next_handle++;
            m_open_devices[handle] = sdl_device;
            m_device_specs[handle] = obtained_spec;
            m_device_gains[handle] = 1.0f;  // Default gain
        }

        return handle;
    }

    void sdl2_backend::close_device(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        
        auto it = m_open_devices.find(device_handle);
        if (it != m_open_devices.end()) {
            SDL_CloseAudioDevice(it->second);
            m_open_devices.erase(it);
            m_device_specs.erase(device_handle);
            m_device_gains.erase(device_handle);
        }
        // Silently ignore invalid handles - this matches original behavior
        // and prevents crashes during cleanup
    }

    audio_format sdl2_backend::get_device_format(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_device_specs.find(device_handle);
        if (it != m_device_specs.end()) {
            return it->second.format;
        }
        THROW_RUNTIME("Invalid device handle");
    }

    sample_rate_t sdl2_backend::get_device_frequency(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_device_specs.find(device_handle);
        if (it != m_device_specs.end()) {
            return it->second.freq;
        }
        THROW_RUNTIME("Invalid device handle");
    }

    channels_t sdl2_backend::get_device_channels(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_device_specs.find(device_handle);
        if (it != m_device_specs.end()) {
            return it->second.channels;
        }
        THROW_RUNTIME("Invalid device handle");
    }

    float sdl2_backend::get_device_gain(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_device_gains.find(device_handle);
        if (it == m_device_gains.end()) {
            THROW_RUNTIME("Invalid device handle");
        }
        return it->second;
    }

    void sdl2_backend::set_device_gain(uint32_t device_handle, float gain) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_device_gains.find(device_handle);
        if (it == m_device_gains.end()) {
            THROW_RUNTIME("Invalid device handle");
        }
        // SDL2 doesn't actually support per-device gain control,
        // but we track the value for API consistency
        it->second = gain;
    }

    bool sdl2_backend::pause_device(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_open_devices.find(device_handle);
        if (it != m_open_devices.end()) {
            SDL_PauseAudioDevice(it->second, 1);
            return true;
        }
        return false;
    }

    bool sdl2_backend::resume_device(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_open_devices.find(device_handle);
        if (it != m_open_devices.end()) {
            SDL_PauseAudioDevice(it->second, 0);
            return true;
        }
        return false;
    }

    bool sdl2_backend::is_device_paused(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_open_devices.find(device_handle);
        if (it != m_open_devices.end()) {
            SDL_AudioStatus status = SDL_GetAudioDeviceStatus(it->second);
            return status == SDL_AUDIO_PAUSED;
        }
        return false;
    }

    std::unique_ptr<audio_stream_interface> sdl2_backend::create_stream(
        uint32_t device_handle,
        const audio_spec& spec,
        void (*callback)(void* userdata, uint8_t* stream, int len),
        void* userdata) {
        
        SDL_AudioDeviceID sdl_device = get_sdl_device(device_handle);
        if (sdl_device == 0) {
            THROW_RUNTIME("Invalid device handle");
        }
        
        return std::make_unique<sdl2_audio_stream>(sdl_device, spec, callback, userdata);
    }

    bool sdl2_backend::supports_recording() const {
        return true;
    }

    int sdl2_backend::get_max_open_devices() const {
        return 32; // Reasonable limit for SDL2
    }

    SDL_AudioDeviceID sdl2_backend::get_sdl_device(uint32_t handle) const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_devices_mutex));
        auto it = m_open_devices.find(handle);
        if (it != m_open_devices.end()) {
            return it->second;
        }
        return 0;
    }

} // namespace musac