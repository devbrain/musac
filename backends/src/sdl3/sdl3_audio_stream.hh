#ifndef MUSAC_SDL3_AUDIO_STREAM_HH
#define MUSAC_SDL3_AUDIO_STREAM_HH

#include <musac/sdk/audio_stream_interface.hh>
#include <musac/sdk/audio_format.hh>
#include "sdl3.hh"
#include <memory>

namespace musac {

class sdl3_audio_stream : public audio_stream_interface {
public:
    sdl3_audio_stream(SDL_AudioDeviceID device_id, const audio_spec& spec,
                     void (*callback)(void* userdata, uint8_t* stream, int len),
                     void* userdata);
    ~sdl3_audio_stream() override;
    
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
    static void sdl_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
    static SDL_AudioFormat to_sdl_format(audio_format fmt);
    
    SDL_AudioDeviceID m_device_id;
    std::shared_ptr<SDL_AudioStream> m_stream;
    void (*m_callback)(void* userdata, uint8_t* stream, int len);
    void* m_userdata;
    bool m_bound;
};

} // namespace musac

#endif // MUSAC_SDL3_AUDIO_STREAM_HH