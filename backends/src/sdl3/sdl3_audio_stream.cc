#include "sdl3_audio_stream.hh"
#include <algorithm>
#include <failsafe/failsafe.hh>

namespace musac {

// Custom deleter that checks if SDL is still initialized
static void safe_destroy_audio_stream(SDL_AudioStream* stream) {
    if (stream && SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_DestroyAudioStream(stream);
    }
    // If SDL is not initialized, we can't safely destroy the stream
    // SDL_Quit() should have cleaned it up already
}

SDL_AudioFormat sdl3_audio_stream::to_sdl_format(audio_format fmt) {
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

sdl3_audio_stream::sdl3_audio_stream(SDL_AudioDeviceID device_id, const audio_spec& spec,
                                   void (*callback)(void* userdata, uint8_t* stream, int len),
                                   void* userdata)
    : m_device_id(device_id)
    , m_callback(callback)
    , m_userdata(userdata)
    , m_bound(false) {
    
    SDL_AudioSpec sdl_spec;
    sdl_spec.format = to_sdl_format(spec.format);
    sdl_spec.channels = spec.channels;
    sdl_spec.freq = static_cast<int>(spec.freq);
    
    if (m_callback) {
        // We already have an opened device, so we need to:
        // 1. Get the device format
        // 2. Create a stream that converts from our format to device format
        // 3. Set up a callback
        // 4. Bind the stream to the device
        
        SDL_AudioSpec device_spec;
        if (!SDL_GetAudioDeviceFormat(m_device_id, &device_spec, nullptr)) {
            THROW_RUNTIME("Failed to get device format");
        }
        
        // Create a stream that converts from our source format to device format
        m_stream = std::shared_ptr<SDL_AudioStream>(
            SDL_CreateAudioStream(&sdl_spec, &device_spec),
            safe_destroy_audio_stream
        );
        
        if (!m_stream) {
            THROW_RUNTIME("Failed to create audio stream");
        }
        
        // Set the callback for when the stream needs data
        if (!SDL_SetAudioStreamGetCallback(m_stream.get(), sdl_callback, this)) {
            THROW_RUNTIME("Failed to set stream callback");
        }
        
        // Bind the stream to the device
        if (!SDL_BindAudioStream(m_device_id, m_stream.get())) {
            THROW_RUNTIME("Failed to bind stream to device");
        }
        m_bound = true;
    } else {
        // Get device spec for creating compatible stream
        SDL_AudioSpec device_spec;
        SDL_GetAudioDeviceFormat(m_device_id, &device_spec, nullptr);
        
        m_stream = std::shared_ptr<SDL_AudioStream>(
            SDL_CreateAudioStream(&sdl_spec, &device_spec),
            safe_destroy_audio_stream
        );
    }
}

sdl3_audio_stream::~sdl3_audio_stream() = default;

void sdl3_audio_stream::sdl_callback(void* userdata,
    SDL_AudioStream* stream,
    int additional_amount, [[maybe_unused]] int total_amount) {
    auto* self = static_cast<sdl3_audio_stream*>(userdata);
    if (!self) {
        return;
    }
    
    if (self->m_callback && additional_amount > 0) {
        std::vector<uint8_t> buffer(static_cast<size_t>(additional_amount));
        auto* data = buffer.data();
        if (data) {
            self->m_callback(self->m_userdata, data, additional_amount);
            SDL_PutAudioStreamData(stream, data, additional_amount);
        }
    }
}

bool sdl3_audio_stream::put_data(const void* data, size_t sz) {
    return SDL_PutAudioStreamData(m_stream.get(), data, static_cast<int>(sz));
}

size_t sdl3_audio_stream::get_data(void* data, size_t sz) {
    int result = SDL_GetAudioStreamData(m_stream.get(), data, static_cast<int>(sz));
    return result > 0 ? static_cast<size_t>(result) : 0;
}

void sdl3_audio_stream::clear() {
    SDL_ClearAudioStream(m_stream.get());
}

bool sdl3_audio_stream::pause() {
    return SDL_PauseAudioStreamDevice(m_stream.get());
}

bool sdl3_audio_stream::resume() {
    return SDL_ResumeAudioStreamDevice(m_stream.get());
}

bool sdl3_audio_stream::is_paused() const {
    return SDL_AudioStreamDevicePaused(m_stream.get());
}

size_t sdl3_audio_stream::get_queued_size() const {
    int result = SDL_GetAudioStreamQueued(m_stream.get());
    return result > 0 ? static_cast<size_t>(result) : 0;
}

bool sdl3_audio_stream::bind_to_device() {
    if (!m_bound && m_stream) {
        if (SDL_BindAudioStream(m_device_id, m_stream.get())) {
            m_bound = true;
            return true;
        }
    }
    return false;
}

void sdl3_audio_stream::unbind_from_device() {
    if (m_bound && m_stream) {
        // LOG_INFO("SDL3Stream", "Unbinding stream from device");
        SDL_UnbindAudioStream(m_stream.get());
        m_bound = false;
        // LOG_INFO("SDL3Stream", "Stream unbound successfully");
    }
}

} // namespace musac