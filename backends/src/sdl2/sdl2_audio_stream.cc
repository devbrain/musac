#include "sdl2_audio_stream.hh"
#include "sdl2_backend_impl.hh"
#include <algorithm>
#include <cstring>
#include <failsafe/failsafe.hh>

namespace musac {

sdl2_audio_stream::sdl2_audio_stream(sdl2_backend* backend, SDL_AudioDeviceID device_id, const audio_spec& spec,
                                   void (*callback)(void* userdata, uint8_t* stream, int len),
                                   void* userdata)
    : m_backend(backend)
    , m_device_id(device_id)
    , m_user_callback(callback)
    , m_userdata(userdata)
    , m_bound(false)
    , m_paused(false)
    , m_buffer_read_pos(0)
    , m_buffer_write_pos(0)
    , m_use_queue_mode(callback == nullptr) {
    
    // Store the spec
    SDL_zero(m_spec);
    m_spec.freq = static_cast<int>(spec.freq);
    m_spec.channels = spec.channels;
    
    // Convert format
    switch (spec.format) {
        case audio_format::u8:     m_spec.format = AUDIO_U8; break;
        case audio_format::s8:     m_spec.format = AUDIO_S8; break;
        case audio_format::s16le:  m_spec.format = AUDIO_S16LSB; break;
        case audio_format::s16be:  m_spec.format = AUDIO_S16MSB; break;
        case audio_format::s32le:  m_spec.format = AUDIO_S32LSB; break;
        case audio_format::s32be:  m_spec.format = AUDIO_S32MSB; break;
        case audio_format::f32le:  m_spec.format = AUDIO_F32LSB; break;
        case audio_format::f32be:  m_spec.format = AUDIO_F32MSB; break;
        default:                   m_spec.format = AUDIO_S16LSB; break;
    }
    
    if (m_user_callback) {
        // Register the callback with the backend
        m_backend->register_stream_callback(m_device_id, sdl_callback, this);
        m_use_queue_mode = false;
    }
}

sdl2_audio_stream::~sdl2_audio_stream() {
    // Inline unbind_from_device logic to avoid virtual call
    if (m_bound) {
        // Clear any queued audio
        if (m_use_queue_mode) {
            SDL_ClearQueuedAudio(m_device_id);
        }
        // Pause the device
        SDL_PauseAudioDevice(m_device_id, 1);
        m_bound = false;
    }
    
    // Don't unregister callback in destructor - this can cause use-after-free
    // if backend is already destroyed. SDL will clean up callbacks when
    // the device is closed anyway.
}

void sdl2_audio_stream::sdl_callback(void* userdata, Uint8* stream, int len) {
    auto* self = static_cast<sdl2_audio_stream*>(userdata);
    if (!self || !self->m_user_callback) {
        // Fill with silence if no callback
        std::memset(stream, 0, static_cast<size_t>(len));
        return;
    }
    
    // Call user callback to fill the buffer
    self->m_user_callback(self->m_userdata, stream, len);
}

bool sdl2_audio_stream::put_data(const void* data, size_t size) {
    if (m_use_queue_mode) {
        // Queue audio directly to the device
        return SDL_QueueAudio(m_device_id, data, static_cast<Uint32>(size)) == 0;
    } else {
        // Buffer mode for callback
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        
        size_t available = m_buffer.size() - m_buffer_write_pos;
        if (available < size) {
            // Buffer full
            return false;
        }
        
        std::memcpy(m_buffer.data() + m_buffer_write_pos, data, size);
        m_buffer_write_pos += size;
        
        return true;
    }
}

size_t sdl2_audio_stream::get_data(void* data, size_t size) {
    if (m_user_callback) {
        // Generate data using callback
        m_user_callback(m_userdata, static_cast<uint8_t*>(data), static_cast<int>(size));
        return size;
    } else if (!m_use_queue_mode) {
        // Read from buffer
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        
        size_t available = m_buffer_write_pos - m_buffer_read_pos;
        size_t to_read = std::min(size, available);
        
        if (to_read > 0) {
            std::memcpy(data, m_buffer.data() + m_buffer_read_pos, to_read);
            m_buffer_read_pos += to_read;
            
            // Reset positions if buffer is empty
            if (m_buffer_read_pos == m_buffer_write_pos) {
                m_buffer_read_pos = 0;
                m_buffer_write_pos = 0;
            }
        }
        
        return to_read;
    }
    
    return 0;
}

void sdl2_audio_stream::clear() {
    if (m_use_queue_mode) {
        SDL_ClearQueuedAudio(m_device_id);
    } else {
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        m_buffer_read_pos = 0;
        m_buffer_write_pos = 0;
    }
}

bool sdl2_audio_stream::pause() {
    SDL_PauseAudioDevice(m_device_id, 1);
    m_paused = true;
    return true;
}

bool sdl2_audio_stream::resume() {
    SDL_PauseAudioDevice(m_device_id, 0);
    m_paused = false;
    return true;
}

bool sdl2_audio_stream::is_paused() const {
    return m_paused;
}

size_t sdl2_audio_stream::get_queued_size() const {
    if (m_use_queue_mode) {
        return SDL_GetQueuedAudioSize(m_device_id);
    } else {
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        return m_buffer_write_pos - m_buffer_read_pos;
    }
}

bool sdl2_audio_stream::bind_to_device() {
    if (!m_bound) {
        // In SDL2, there's no explicit bind - just resume the device
        SDL_PauseAudioDevice(m_device_id, 0);
        m_bound = true;
        return true;
    }
    return false;
}

void sdl2_audio_stream::unbind_from_device() {
    if (m_bound) {
        // Clear any queued audio
        clear();
        // Pause the device
        SDL_PauseAudioDevice(m_device_id, 1);
        m_bound = false;
    }
}

} // namespace musac