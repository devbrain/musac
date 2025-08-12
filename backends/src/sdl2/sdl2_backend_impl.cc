#include "sdl2_backend_impl.hh"
#include "sdl2_audio_stream.hh"
#include <musac/sdk/audio_stream_interface.hh>
#include <failsafe/failsafe.hh>
#include <algorithm>
#include <cstring>

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
            for (const auto& [handle, info] : m_devices) {
                SDL_CloseAudioDevice(info.sdl_id);
            }
            m_devices.clear();
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

    std::vector<device_info> sdl2_backend::enumerate_devices(bool playback) {
        if (!m_initialized) {
            THROW_RUNTIME("Backend not initialized");
        }

        // SDL2 audio functions may not be thread-safe, protect with mutex
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        
        std::vector<device_info> devices;

        // Get device count
        int count = SDL_GetNumAudioDevices(playback ? 0 : 1);
        
        // Get the default device info using SDL_GetDefaultAudioInfo
        char* default_device_name = nullptr;
        SDL_AudioSpec default_spec;
        SDL_zero(default_spec);  // Initialize the spec structure
        
        int default_result = SDL_GetDefaultAudioInfo(&default_device_name, &default_spec, playback ? 0 : 1);
        
        std::string default_name_str;
        if (default_result == 0 && default_device_name) {
            default_name_str = default_device_name;
            // Free immediately after copying to avoid any leaks
            SDL_free(default_device_name);
            default_device_name = nullptr;
        } else if (default_device_name) {
            // Still free even if the call failed
            SDL_free(default_device_name);
            default_device_name = nullptr;
        }
        
        // Collect all devices
        for (int i = 0; i < count; i++) {
            const char* name = SDL_GetAudioDeviceName(i, playback ? 0 : 1);
            if (!name) continue;

            device_info info;
            info.name = name;
            info.id = std::to_string(i);
            info.is_default = false;
            // SDL2 doesn't provide device specs without opening, use defaults
            info.channels = 2;
            info.sample_rate = 44100;
            devices.push_back(info);
        }

        // Find and mark the default device
        int default_index = -1;
        if (!default_name_str.empty()) {
            for (size_t i = 0; i < devices.size(); ++i) {
                if (devices[i].name == default_name_str) {
                    devices[i].is_default = true;
                    default_index = static_cast<int>(i);
                    // Use the actual default device specs
                    if (default_result == 0) {
                        devices[i].channels = default_spec.channels;
                        devices[i].sample_rate = static_cast<uint32_t>(default_spec.freq);
                    }
                    break;
                }
            }
        }
        
        // If no match found but we have devices, mark first as default
        if (default_index == -1 && !devices.empty()) {
            devices[0].is_default = true;
            default_index = 0;
        }
        
        // If we found a default device that's not first, move it to the front
        if (default_index > 0) {
            std::swap(devices[0], devices[static_cast<std::size_t>(default_index)]);
            // Don't update the device IDs - they should remain as the original SDL indices
            // The IDs represent the actual SDL device indices, not our list positions
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

    device_info sdl2_backend::get_default_device(bool playback) {
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

    uint32_t sdl2_backend::open_device(const std::string& device_id,
                                       const audio_spec& spec,
                                       audio_spec& obtained_spec) {
        if (!m_initialized) {
            THROW_RUNTIME("Backend not initialized");
        }

        // Note: We always open a new device handle, even for the same physical device
        // This matches the expected behavior where each open_device call returns a unique handle

        // Prepare SDL audio spec
        SDL_AudioSpec wanted;
        SDL_zero(wanted);
        wanted.freq = static_cast<int>(spec.freq);
        wanted.format = musac_to_sdl_format(spec.format);
        wanted.channels = spec.channels;
        wanted.samples = 4096; // SDL2 default buffer size
        
        // Create callback data for this device
        auto callback_data = std::make_unique<device_callback_data>();
        wanted.callback = audio_callback;
        wanted.userdata = callback_data.get();

        SDL_AudioSpec obtained;
        
        // Parse device name/index
        const char* device_name = nullptr;
        if (!device_id.empty() && device_id != "default") {
            // Device ID is an index string, convert to int and get the device name
            try {
                int device_index = std::stoi(device_id);
                device_name = SDL_GetAudioDeviceName(device_index, 0); // 0 for playback
                if (!device_name) {
                    THROW_RUNTIME("Invalid device index: " + device_id);
                }
            } catch (const std::exception&) {
                // If it's not a number, treat it as a device name
                device_name = device_id.c_str();
            }
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
        
        // SDL2 opens devices with callbacks in a paused state
        // Unpause it to match the expected behavior (devices should start unpaused)
        SDL_PauseAudioDevice(sdl_device, 0);

        // Fill obtained spec
        obtained_spec.freq = static_cast<uint32_t>(obtained.freq);
        obtained_spec.format = sdl_to_musac_format(obtained.format);
        obtained_spec.channels = obtained.channels;

        // Store device info
        uint32_t handle;
        {
            std::lock_guard<std::mutex> lock(m_devices_mutex);
            handle = m_next_handle++;
            
            // Create and populate device state
            device_state& info = m_devices[handle];
            info.sdl_id = sdl_device;
            info.spec = obtained_spec;
            info.gain = 1.0f;  // Default gain
            info.is_muted = false;  // Default not muted
            info.callback_data = std::move(callback_data);
        }

        return handle;
    }

    void sdl2_backend::close_device(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            SDL_CloseAudioDevice(it->second.sdl_id);
            m_devices.erase(it);
        }
        // Silently ignore invalid handles - this matches original behavior
        // and prevents crashes during cleanup
    }

    audio_format sdl2_backend::get_device_format(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            return it->second.spec.format;
        }
        THROW_RUNTIME("Invalid device handle");
    }

    sample_rate_t sdl2_backend::get_device_frequency(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            return it->second.spec.freq;
        }
        THROW_RUNTIME("Invalid device handle");
    }

    channels_t sdl2_backend::get_device_channels(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            return it->second.spec.channels;
        }
        THROW_RUNTIME("Invalid device handle");
    }

    float sdl2_backend::get_device_gain(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }
        return it->second.gain;
    }

    void sdl2_backend::set_device_gain(uint32_t device_handle, float gain) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it == m_devices.end()) {
            THROW_RUNTIME("Invalid device handle");
        }
        // SDL2 doesn't actually support per-device gain control,
        // but we track the value for API consistency
        it->second.gain = gain;
    }

    bool sdl2_backend::pause_device(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            SDL_PauseAudioDevice(it->second.sdl_id, 1);
            return true;
        }
        return false;
    }

    bool sdl2_backend::resume_device(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            SDL_PauseAudioDevice(it->second.sdl_id, 0);
            return true;
        }
        return false;
    }

    bool sdl2_backend::is_device_paused(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            SDL_AudioStatus status = SDL_GetAudioDeviceStatus(it->second.sdl_id);
            return status == SDL_AUDIO_PAUSED;
        }
        return false;
    }

    bool sdl2_backend::supports_mute() const {
        return true; // SDL2 supports pause which we use for muting
    }

    bool sdl2_backend::mute_device(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            // Track mute state
            it->second.is_muted = true;
            // Use SDL pause to implement mute
            SDL_PauseAudioDevice(it->second.sdl_id, 1);
            return true;
        }
        return false;
    }

    bool sdl2_backend::unmute_device(uint32_t device_handle) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            // Clear mute state
            it->second.is_muted = false;
            // Resume audio playback
            SDL_PauseAudioDevice(it->second.sdl_id, 0);
            return true;
        }
        return false;
    }

    bool sdl2_backend::is_device_muted(uint32_t device_handle) const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_devices_mutex));
        auto it = m_devices.find(device_handle);
        if (it != m_devices.end()) {
            return it->second.is_muted;
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
        
        return std::make_unique<sdl2_audio_stream>(this, sdl_device, spec, callback, userdata);
    }

    bool sdl2_backend::supports_recording() const {
        return true;
    }

    int sdl2_backend::get_max_open_devices() const {
        return 32; // Reasonable limit for SDL2
    }

    SDL_AudioDeviceID sdl2_backend::get_sdl_device(uint32_t handle) const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_devices_mutex));
        auto it = m_devices.find(handle);
        if (it != m_devices.end()) {
            return it->second.sdl_id;
        }
        return 0;
    }
    
    void sdl2_backend::audio_callback(void* userdata, Uint8* stream, int len) {
        auto* callback_data = static_cast<device_callback_data*>(userdata);
        if (!callback_data) {
            // Fill with silence if no callback data
            std::memset(stream, 0, static_cast<size_t>(len));
            return;
        }
        
        std::lock_guard<std::mutex> lock(callback_data->mutex);
        if (callback_data->callback) {
            // Call the registered stream callback
            callback_data->callback(callback_data->userdata, stream, len);
        } else {
            // Fill with silence if no callback registered
            std::memset(stream, 0, static_cast<size_t>(len));
        }
    }
    
    void sdl2_backend::register_stream_callback(SDL_AudioDeviceID device_id,
                                               void (*callback)(void* userdata, uint8_t* stream, int len),
                                               void* userdata) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        // Find the device by SDL ID
        for (auto& [handle, info] : m_devices) {
            if (info.sdl_id == device_id && info.callback_data) {
                std::lock_guard<std::mutex> cb_lock(info.callback_data->mutex);
                info.callback_data->callback = callback;
                info.callback_data->userdata = userdata;
                break;
            }
        }
    }
    
    void sdl2_backend::unregister_stream_callback(SDL_AudioDeviceID device_id) {
        std::lock_guard<std::mutex> lock(m_devices_mutex);
        // Find the device by SDL ID
        for (auto& [handle, info] : m_devices) {
            if (info.sdl_id == device_id && info.callback_data) {
                std::lock_guard<std::mutex> cb_lock(info.callback_data->mutex);
                info.callback_data->callback = nullptr;
                info.callback_data->userdata = nullptr;
                break;
            }
        }
    }

} // namespace musac