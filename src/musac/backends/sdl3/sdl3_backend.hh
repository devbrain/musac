#ifndef MUSAC_SDL3_BACKEND_V2_HH
#define MUSAC_SDL3_BACKEND_V2_HH

#include <musac/sdk/audio_backend.hh>
#include <SDL3/SDL.h>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace musac {


/**
 * SDL3 implementation of the unified audio backend interface.
 * This backend provides full device management and audio streaming using SDL3.
 */
class sdl3_backend : public audio_backend {
private:
    bool m_initialized = false;
    std::map<uint32_t, SDL_AudioDeviceID> m_open_devices;
    std::map<uint32_t, audio_spec> m_device_specs;
    std::mutex m_devices_mutex;
    uint32_t m_next_handle = 1;
    
    // Helper to convert SDL format to musac format
    static audio_format sdl_to_musac_format(SDL_AudioFormat sdl_fmt);
    static SDL_AudioFormat musac_to_sdl_format(audio_format fmt);
    
public:
    sdl3_backend() = default;
    ~sdl3_backend() override;
    
    // Initialization
    void init() override;
    void shutdown() override;
    std::string get_name() const override;
    bool is_initialized() const override;
    
    // Device enumeration
    std::vector<device_info_v2> enumerate_devices(bool playback) override;
    device_info_v2 get_default_device(bool playback) override;
    
    // Device management
    uint32_t open_device(const std::string& device_id, 
                        const audio_spec& spec, 
                        audio_spec& obtained_spec) override;
    void close_device(uint32_t device_handle) override;
    
    // Device properties
    audio_format get_device_format(uint32_t device_handle) override;
    sample_rate_t get_device_frequency(uint32_t device_handle) override;
    channels_t get_device_channels(uint32_t device_handle) override;
    float get_device_gain(uint32_t device_handle) override;
    void set_device_gain(uint32_t device_handle, float gain) override;
    
    // Device control
    bool pause_device(uint32_t device_handle) override;
    bool resume_device(uint32_t device_handle) override;
    bool is_device_paused(uint32_t device_handle) override;
    
    // Stream creation
    std::unique_ptr<audio_stream_interface> create_stream(
        uint32_t device_handle,
        const audio_spec& spec,
        void (*callback)(void* userdata, uint8_t* stream, int len),
        void* userdata) override;
    
    // Backend capabilities
    bool supports_recording() const override;
    int get_max_open_devices() const override;
    
    // SDL3-specific helper
    SDL_AudioDeviceID get_sdl_device(uint32_t handle) const;
};

} // namespace musac

#endif // MUSAC_SDL3_BACKEND_V2_HH