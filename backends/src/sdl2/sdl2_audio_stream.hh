#ifndef MUSAC_SDL2_AUDIO_STREAM_HH
#define MUSAC_SDL2_AUDIO_STREAM_HH

#include <musac/sdk/audio_stream_interface.hh>
#include <musac/sdk/audio_format.hh>
#include "sdl2.hh"
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

namespace musac {

// Forward declaration
class sdl2_backend;

class sdl2_audio_stream : public audio_stream_interface {
public:
    sdl2_audio_stream(sdl2_backend* backend, SDL_AudioDeviceID device_id, const audio_spec& spec,
                     void (*callback)(void* userdata, uint8_t* stream, int len),
                     void* userdata);
    ~sdl2_audio_stream() override;
    
    bool put_data(const void* data, size_t size) override;
    size_t get_data(void* data, size_t size) override;
    void clear() override;
    bool pause() override;
    bool resume() override;
    bool is_paused() const override;
    size_t get_queued_size() const override;
    bool bind_to_device() override;
    void unbind_from_device() override;
    
private:
    static void sdl_callback(void* userdata, Uint8* stream, int len);
    
    sdl2_backend* m_backend;
    SDL_AudioDeviceID m_device_id;
    SDL_AudioSpec m_spec;
    void (*m_user_callback)(void* userdata, uint8_t* stream, int len);
    void* m_userdata;
    std::atomic<bool> m_bound;
    std::atomic<bool> m_paused;
    
    // Audio buffer for callback mode
    std::vector<uint8_t> m_buffer;
    size_t m_buffer_read_pos;
    size_t m_buffer_write_pos;
    mutable std::mutex m_buffer_mutex;
    
    // For queue mode (when no callback is provided)
    bool m_use_queue_mode;
};

} // namespace musac

#endif // MUSAC_SDL2_AUDIO_STREAM_HH