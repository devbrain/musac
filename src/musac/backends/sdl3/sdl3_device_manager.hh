#ifndef MUSAC_SDL3_DEVICE_MANAGER_HH
#define MUSAC_SDL3_DEVICE_MANAGER_HH

#include <musac/audio_device_interface.hh>
#include <SDL3/SDL.h>

namespace musac {

class sdl3_device_manager : public audio_device_interface {
public:
    sdl3_device_manager();
    ~sdl3_device_manager() override;
    
    std::vector<device_info> enumerate_devices(bool playback = true) override;
    device_info get_default_device(bool playback = true) override;
    uint32_t open_device(const std::string& device_id, const audio_spec& spec, audio_spec& obtained_spec) override;
    void close_device(uint32_t device_handle) override;
    audio_format get_device_format(uint32_t device_handle) override;
    int get_device_frequency(uint32_t device_handle) override;
    int get_device_channels(uint32_t device_handle) override;
    float get_device_gain(uint32_t device_handle) override;
    void set_device_gain(uint32_t device_handle, float gain) override;
    bool pause_device(uint32_t device_handle) override;
    bool resume_device(uint32_t device_handle) override;
    bool is_device_paused(uint32_t device_handle) override;
    std::unique_ptr<audio_stream_interface> create_stream(
        uint32_t device_handle,
        const audio_spec& spec,
        void (*callback)(void* userdata, uint8_t* stream, int len),
        void* userdata
    ) override;
    
private:
    // Convert between SDL and musac audio formats
    static SDL_AudioFormat to_sdl_format(audio_format fmt);
    static audio_format from_sdl_format(SDL_AudioFormat fmt);
    
    // No mapping needed - SDL_AudioDeviceID is already uint32_t
};

} // namespace musac

#endif // MUSAC_SDL3_DEVICE_MANAGER_HH