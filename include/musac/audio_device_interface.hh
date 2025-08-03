#ifndef MUSAC_AUDIO_DEVICE_INTERFACE_HH
#define MUSAC_AUDIO_DEVICE_INTERFACE_HH

#include <vector>
#include <string>
#include <memory>
#include <musac/export_musac.h>
#include <musac/sdk/audio_format.h>

namespace musac {

// Forward declarations
class audio_stream_interface;

/**
 * Device information structure
 */
struct MUSAC_EXPORT device_info {
    std::string name;
    std::string id;
    bool is_default;
    int channels;
    int sample_rate;
};


/**
 * Abstract interface for audio device management.
 * Implementations handle platform-specific device operations.
 */
class MUSAC_EXPORT audio_device_interface {
public:
    virtual ~audio_device_interface() = default;
    
    /**
     * Enumerate available audio devices.
     * @param playback If true, enumerate playback devices; if false, recording devices
     * @return Vector of available devices
     */
    virtual std::vector<device_info> enumerate_devices(bool playback = true) = 0;
    
    /**
     * Get the default device.
     * @param playback If true, get default playback device; if false, default recording device
     * @return Default device info
     */
    virtual device_info get_default_device(bool playback = true) = 0;
    
    /**
     * Open an audio device.
     * @param device_id Device identifier (empty string for default)
     * @param spec Desired audio specification
     * @param obtained_spec Actual specification obtained (output parameter)
     * @return Device handle on success, 0 on failure
     */
    virtual uint32_t open_device(const std::string& device_id, const audio_spec& spec, audio_spec& obtained_spec) = 0;
    
    /**
     * Close an audio device.
     * @param device_handle Device handle returned by open_device
     */
    virtual void close_device(uint32_t device_handle) = 0;
    
    /**
     * Get device audio format.
     * @param device_handle Device handle
     * @return Audio format
     */
    virtual audio_format get_device_format(uint32_t device_handle) = 0;
    
    /**
     * Get device sample rate.
     * @param device_handle Device handle
     * @return Sample rate in Hz
     */
    virtual int get_device_frequency(uint32_t device_handle) = 0;
    
    /**
     * Get device channel count.
     * @param device_handle Device handle
     * @return Number of channels
     */
    virtual int get_device_channels(uint32_t device_handle) = 0;
    
    /**
     * Get device gain/volume.
     * @param device_handle Device handle
     * @return Gain value (0.0 to 1.0)
     */
    virtual float get_device_gain(uint32_t device_handle) = 0;
    
    /**
     * Set device gain/volume.
     * @param device_handle Device handle
     * @param gain Gain value (0.0 to 1.0)
     */
    virtual void set_device_gain(uint32_t device_handle, float gain) = 0;
    
    /**
     * Create an audio stream for the device.
     * @param device_handle Device handle
     * @param spec Audio specification for the stream
     * @param callback Optional callback for audio data
     * @param userdata Optional user data for callback
     * @return Audio stream interface
     */
    virtual std::unique_ptr<audio_stream_interface> create_stream(
        uint32_t device_handle,
        const audio_spec& spec,
        void (*callback)(void* userdata, uint8_t* stream, int len) = nullptr,
        void* userdata = nullptr
    ) = 0;
};

/**
 * Factory function to create the default audio device manager.
 * This can be configured at build time.
 */
MUSAC_EXPORT std::unique_ptr<audio_device_interface> create_default_audio_device_manager();

} // namespace musac

#endif // MUSAC_AUDIO_DEVICE_INTERFACE_HH