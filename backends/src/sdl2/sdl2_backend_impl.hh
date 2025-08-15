/**
 * @file sdl2_backend_impl.hh
 * @brief SDL2 backend implementation
 * @ingroup sdl2_backend
 */

#ifndef MUSAC_SDL2_BACKEND_IMPL_HH
#define MUSAC_SDL2_BACKEND_IMPL_HH

#include <musac/sdk/audio_backend.hh>
#include "sdl2.hh"
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace musac {

/**
 * @class sdl2_backend
 * @brief SDL2 implementation of the audio backend interface
 * @ingroup sdl2_backend
 * 
 * This class provides a complete implementation of the audio_backend
 * interface using SDL2's audio subsystem. It manages device lifecycle,
 * audio callbacks, and format conversion between musac and SDL2.
 * 
 * ## Implementation Details
 * 
 * - **Device Management**: Maps musac handles to SDL2 device IDs
 * - **Callback System**: Bridges SDL2 callbacks to musac streams
 * - **Format Conversion**: Automatic conversion between formats
 * - **Thread Safety**: Mutex-protected device operations
 * 
 * ## SDL2 Integration
 * 
 * The backend uses SDL2's audio subsystem but does not require
 * SDL_Init(SDL_INIT_AUDIO) to be called beforehand - it manages
 * SDL2 audio initialization internally.
 * 
 * @note This is an internal implementation class. Users should
 *       create instances via create_sdl2_backend().
 */
class sdl2_backend : public audio_backend {
private:
    bool m_initialized = false;
    
    // Callback management for SDL2
    struct device_callback_data {
        void (*callback)(void* userdata, uint8_t* stream, int len) = nullptr;
        void* userdata = nullptr;
        std::mutex mutex;
    };
    
    // Aggregated device information
    struct device_state {
        SDL_AudioDeviceID sdl_id = 0;
        audio_spec spec;
        float gain = 1.0f;
        bool is_muted = false;
        std::unique_ptr<device_callback_data> callback_data;
    };
    
    std::map<uint32_t, device_state> m_devices;  // All device info in one place
    std::mutex m_devices_mutex;
    uint32_t m_next_handle = 1;
    
    // Static callback that dispatches to the correct stream
    static void audio_callback(void* userdata, Uint8* stream, int len);
    
    // Helper to convert SDL format to musac format
    static audio_format sdl_to_musac_format(SDL_AudioFormat sdl_fmt);
    static SDL_AudioFormat musac_to_sdl_format(audio_format fmt);
    
public:
    sdl2_backend() = default;
    ~sdl2_backend() override;
    
    // Initialization
    void init() override;
    void shutdown() override;
    std::string get_name() const override;
    bool is_initialized() const override;
    
    // Device enumeration
    std::vector<musac::device_info> enumerate_devices(bool playback) override;
    musac::device_info get_default_device(bool playback) override;
    
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
    
    // Mute control (using SDL pause as implementation)
    bool supports_mute() const override;
    bool mute_device(uint32_t device_handle) override;
    bool unmute_device(uint32_t device_handle) override;
    bool is_device_muted(uint32_t device_handle) const override;
    
    // Stream creation
    std::unique_ptr<audio_stream_interface> create_stream(
        uint32_t device_handle,
        const audio_spec& spec,
        void (*callback)(void* userdata, uint8_t* stream, int len),
        void* userdata) override;
    
    // Backend capabilities
    bool supports_recording() const override;
    int get_max_open_devices() const override;
    
    // SDL2-specific helpers
    SDL_AudioDeviceID get_sdl_device(uint32_t handle) const;
    void register_stream_callback(SDL_AudioDeviceID device_id, 
                                 void (*callback)(void* userdata, uint8_t* stream, int len),
                                 void* userdata);
    void unregister_stream_callback(SDL_AudioDeviceID device_id);
};

} // namespace musac

#endif // MUSAC_SDL2_BACKEND_IMPL_HH