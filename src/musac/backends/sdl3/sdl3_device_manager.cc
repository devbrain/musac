#include "sdl3_device_manager.hh"
#include "sdl3_audio_stream.hh"
#include <musac/error.hh>
#include <failsafe/failsafe.hh>

namespace musac {

sdl3_device_manager::sdl3_device_manager() {}

sdl3_device_manager::~sdl3_device_manager() {
    // SDL automatically closes all devices when SDL_Quit is called
    // No manual cleanup needed since we don't maintain a device list
}

SDL_AudioFormat sdl3_device_manager::to_sdl_format(audio_format fmt) {
    switch (fmt) {
        case audio_format::u8:     return SDL_AUDIO_U8;
        case audio_format::s8:     return SDL_AUDIO_S8;
        case audio_format::s16le:  return SDL_AUDIO_S16LE;
        case audio_format::s16be:  return SDL_AUDIO_S16BE;
        case audio_format::s32le:  return SDL_AUDIO_S32LE;
        case audio_format::s32be:  return SDL_AUDIO_S32BE;
        case audio_format::f32le:  return SDL_AUDIO_F32LE;
        case audio_format::f32be:  return SDL_AUDIO_F32BE;
        default:                   return SDL_AUDIO_UNKNOWN;
    }
}

audio_format sdl3_device_manager::from_sdl_format(SDL_AudioFormat fmt) {
    switch (fmt) {
        case SDL_AUDIO_U8:     return audio_format::u8;
        case SDL_AUDIO_S8:     return audio_format::s8;
        case SDL_AUDIO_S16LE:  return audio_format::s16le;
        case SDL_AUDIO_S16BE:  return audio_format::s16be;
        case SDL_AUDIO_S32LE:  return audio_format::s32le;
        case SDL_AUDIO_S32BE:  return audio_format::s32be;
        case SDL_AUDIO_F32LE:  return audio_format::f32le;
        case SDL_AUDIO_F32BE:  return audio_format::f32be;
        default:               return audio_format::unknown;
    }
}

std::vector<device_info> sdl3_device_manager::enumerate_devices(bool playback) {
    std::vector<device_info> result;
    
    int count = 0;
    SDL_AudioDeviceID* ids = playback 
        ? SDL_GetAudioPlaybackDevices(&count)
        : SDL_GetAudioRecordingDevices(&count);
    
    if (!ids) {
        return result;
    }
    
    for (int i = 0; i < count; ++i) {
        device_info info;
        
        const char* name = SDL_GetAudioDeviceName(ids[i]);
        info.name = name ? name : "Unknown Device";
        info.id = std::to_string(ids[i]);
        info.is_default = (i == 0); // SDL returns default first
        
        // Get device spec
        SDL_AudioSpec spec;
        if (SDL_GetAudioDeviceFormat(ids[i], &spec, nullptr)) {
            info.channels = spec.channels;
            info.sample_rate = spec.freq;
        }
        
        result.push_back(info);
    }
    
    SDL_free(ids);
    return result;
}

device_info sdl3_device_manager::get_default_device(bool playback) {
    device_info info;
    
    SDL_AudioDeviceID id = SDL_OpenAudioDevice(
        playback ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK : SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
        nullptr
    );
    
    if (id) {
        const char* name = SDL_GetAudioDeviceName(id);
        info.name = name ? name : "Default Device";
        info.id = "default";
        info.is_default = true;
        
        SDL_AudioSpec spec;
        if (SDL_GetAudioDeviceFormat(id, &spec, nullptr)) {
            info.channels = spec.channels;
            info.sample_rate = spec.freq;
        }
        
        SDL_CloseAudioDevice(id);
    }
    
    return info;
}

uint32_t sdl3_device_manager::open_device(const std::string& device_id, const audio_spec& spec, audio_spec& obtained_spec) {
    SDL_AudioSpec wanted;
    wanted.format = to_sdl_format(spec.format);
    wanted.channels = spec.channels;
    wanted.freq = spec.freq;
    
    SDL_AudioDeviceID sdl_device_id;
    if (device_id.empty() || device_id == "default") {
        sdl_device_id = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    } else {
        sdl_device_id = static_cast<SDL_AudioDeviceID>(std::stoul(device_id));
    }
    
    SDL_AudioDeviceID opened_id = SDL_OpenAudioDevice(sdl_device_id, &wanted);
    if (!opened_id) {
        const char* error = SDL_GetError();
        THROW_RUNTIME("Failed to open audio device: ", error ? error : "unknown error");
    }
    
    // Get the actual spec
    SDL_AudioSpec obtained_sdl_spec;
    if (!SDL_GetAudioDeviceFormat(opened_id, &obtained_sdl_spec, nullptr)) {
        SDL_CloseAudioDevice(opened_id);
        THROW_RUNTIME("Failed to get audio device format");
    }
    
    obtained_spec.format = from_sdl_format(obtained_sdl_spec.format);
    obtained_spec.channels = obtained_sdl_spec.channels;
    obtained_spec.freq = obtained_sdl_spec.freq;
    
    // Check if device is paused after opening
    bool is_paused = SDL_AudioDevicePaused(opened_id);
    // LOG_INFO("SDL3DeviceManager", "Device opened with SDL ID:", opened_id, "Is paused:", is_paused);
    
    // SDL_AudioDeviceID is already a uint32_t, so we can return it directly
    return static_cast<uint32_t>(opened_id);
}

void sdl3_device_manager::close_device(uint32_t device_handle) {
    // Only close if SDL is still initialized and handle is valid
    if (device_handle != 0 && SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_CloseAudioDevice(static_cast<SDL_AudioDeviceID>(device_handle));
    }
}

audio_format sdl3_device_manager::get_device_format(uint32_t device_handle) {
    if (device_handle == 0) {
        return audio_format::unknown;
    }
    
    SDL_AudioSpec spec;
    if (SDL_GetAudioDeviceFormat(static_cast<SDL_AudioDeviceID>(device_handle), &spec, nullptr)) {
        return from_sdl_format(spec.format);
    }
    
    return audio_format::unknown;
}

int sdl3_device_manager::get_device_frequency(uint32_t device_handle) {
    if (device_handle == 0) {
        return 0;
    }
    
    SDL_AudioSpec spec;
    if (SDL_GetAudioDeviceFormat(static_cast<SDL_AudioDeviceID>(device_handle), &spec, nullptr)) {
        return spec.freq;
    }
    
    return 0;
}

int sdl3_device_manager::get_device_channels(uint32_t device_handle) {
    if (device_handle == 0) {
        return 0;
    }
    
    SDL_AudioSpec spec;
    if (SDL_GetAudioDeviceFormat(static_cast<SDL_AudioDeviceID>(device_handle), &spec, nullptr)) {
        return spec.channels;
    }
    
    return 0;
}

float sdl3_device_manager::get_device_gain(uint32_t device_handle) {
    if (device_handle == 0) {
        return 0.0f;
    }
    
    return SDL_GetAudioDeviceGain(static_cast<SDL_AudioDeviceID>(device_handle));
}

void sdl3_device_manager::set_device_gain(uint32_t device_handle, float gain) {
    if (device_handle != 0) {
        SDL_SetAudioDeviceGain(static_cast<SDL_AudioDeviceID>(device_handle), gain);
    }
}

bool sdl3_device_manager::pause_device(uint32_t device_handle) {
    if (device_handle == 0) {
        return false;
    }
    
    // SDL_PauseAudioDevice returns true on success
    return SDL_PauseAudioDevice(static_cast<SDL_AudioDeviceID>(device_handle));
}

bool sdl3_device_manager::resume_device(uint32_t device_handle) {
    if (device_handle == 0) {
        // LOG_ERROR("SDL3DeviceManager", "resume_device: invalid handle", device_handle);
        return false;
    }
    
    // LOG_INFO("SDL3DeviceManager", "Resuming device", device_handle);
    // SDL_ResumeAudioDevice returns true on success
    bool result = SDL_ResumeAudioDevice(static_cast<SDL_AudioDeviceID>(device_handle));
    // LOG_INFO("SDL3DeviceManager", "Resume result:", result);
    return result;
}

bool sdl3_device_manager::is_device_paused(uint32_t device_handle) {
    if (device_handle == 0) {
        return false;
    }
    
    return SDL_AudioDevicePaused(static_cast<SDL_AudioDeviceID>(device_handle));
}

std::unique_ptr<audio_stream_interface> sdl3_device_manager::create_stream(
    uint32_t device_handle,
    const audio_spec& spec,
    void (*callback)(void* userdata, uint8_t* stream, int len),
    void* userdata
) {
    if (device_handle == 0) {
        THROW_RUNTIME("Invalid device handle for stream creation");
    }
    
    return std::make_unique<sdl3_audio_stream>(static_cast<SDL_AudioDeviceID>(device_handle), spec, callback, userdata);
}

} // namespace musac