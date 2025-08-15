/**
 * @file sdl3_backend_impl.hh
 * @brief SDL3 backend implementation
 * @ingroup sdl3_backend
 */

#ifndef MUSAC_SDL3_BACKEND_V2_HH
#define MUSAC_SDL3_BACKEND_V2_HH

#include <musac/sdk/audio_backend.hh>
#include "sdl3.hh"
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace musac {

/**
 * @class sdl3_backend
 * @brief SDL3 implementation of the audio backend interface
 * @ingroup sdl3_backend
 * 
 * This class provides a complete implementation of the audio_backend
 * interface using SDL3's modern audio subsystem. It leverages SDL3's
 * improved audio architecture for better performance and reliability.
 * 
 * ## Key Differences from SDL2 Backend
 * 
 * - **Audio Streams**: Uses SDL3's stream API instead of device callbacks
 * - **Device Management**: Improved device lifecycle handling
 * - **Format Support**: Extended format support including 24-bit audio
 * - **Performance**: Optimized for lower latency and CPU usage
 * 
 * ## Implementation Details
 * 
 * - **Handle Mapping**: Maps musac handles to SDL3 device IDs
 * - **Stream Management**: Each device can have multiple streams
 * - **Format Conversion**: Leverages SDL3's built-in converters
 * - **Thread Safety**: Fine-grained locking for better concurrency
 * 
 * ## SDL3 Audio Architecture
 * 
 * SDL3 introduces a new audio architecture:
 * 1. Devices represent physical audio endpoints
 * 2. Streams provide data paths to devices
 * 3. Improved callback system with timing guarantees
 * 
 * @note This is an internal implementation class. Users should
 *       create instances via create_sdl3_backend().
 */
class sdl3_backend : public audio_backend {
private:
    bool m_initialized = false;
    
    // Aggregated device information
    struct device_state {
        SDL_AudioDeviceID sdl_id = 0;
        audio_spec spec;
        float gain = 1.0f;  // SDL3 may support gain in the future
        bool is_muted = false;
    };
    
    std::map<uint32_t, device_state> m_devices;  // All device info in one place
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
    
    // SDL3-specific helper
    SDL_AudioDeviceID get_sdl_device(uint32_t handle) const;
};

} // namespace musac

#endif // MUSAC_SDL3_BACKEND_V2_HH