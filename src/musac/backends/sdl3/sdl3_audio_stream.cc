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
    sdl_spec.freq = spec.freq;
    
    // LOG_INFO("SDL3Stream", "Creating stream for device", device_id, 
    //          "with callback:", (m_callback != nullptr), 
    //          "format:", static_cast<int>(spec.format), "channels:", spec.channels, "freq:", spec.freq);
    
    if (m_callback) {
        m_stream = std::shared_ptr<SDL_AudioStream>(
            SDL_OpenAudioDeviceStream(m_device_id, &sdl_spec, sdl_callback, this),
            safe_destroy_audio_stream
        );
        // SDL_OpenAudioDeviceStream automatically binds the stream
        m_bound = true;
        // LOG_INFO("SDL3Stream", "Stream created with SDL_OpenAudioDeviceStream, bound:", m_bound);
        
        // Check if stream is paused
        bool stream_paused = SDL_AudioStreamDevicePaused(m_stream.get());
        // LOG_INFO("SDL3Stream", "Stream paused state:", stream_paused);
        
        // Try resuming the stream 
        if (stream_paused) {
            bool resume_result = SDL_ResumeAudioStreamDevice(m_stream.get());
            // LOG_INFO("SDL3Stream", "Stream resume result:", resume_result);
        }
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

sdl3_audio_stream::~sdl3_audio_stream() {
    // LOG_INFO("SDL3Stream", "Destroying audio stream, bound:", m_bound);
    // The stream will be automatically destroyed by the shared_ptr
    // Don't try to unbind - the device might already be closed
}

void sdl3_audio_stream::sdl_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    static int sdl_callback_count = 0;
    // if (++sdl_callback_count % 100 == 0) {
    //     LOG_INFO("SDL3Stream", "SDL callback called", sdl_callback_count, "times, additional:", additional_amount);
    // }
    
    auto* self = static_cast<sdl3_audio_stream*>(userdata);
    if (!self) {
        // LOG_ERROR("SDL3Stream", "Callback called with null userdata!");
        return;
    }
    
    if (self->m_callback && additional_amount > 0) {
        auto* data = SDL_stack_alloc(uint8_t, additional_amount);
        if (data) {
            self->m_callback(self->m_userdata, data, additional_amount);
            SDL_PutAudioStreamData(stream, data, additional_amount);
            SDL_stack_free(data);
        }
    }
}

bool sdl3_audio_stream::put_data(const void* data, size_t size) {
    return SDL_PutAudioStreamData(m_stream.get(), data, static_cast<int>(size));
}

size_t sdl3_audio_stream::get_data(void* data, size_t size) {
    int result = SDL_GetAudioStreamData(m_stream.get(), data, static_cast<int>(size));
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