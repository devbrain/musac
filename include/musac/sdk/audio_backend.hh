#ifndef MUSAC_SDK_AUDIO_BACKEND_V2_HH
#define MUSAC_SDK_AUDIO_BACKEND_V2_HH

#include <string>
#include <memory>
#include <vector>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/types.hh>
#include <musac/audio_stream_interface.hh>

namespace musac {

/**
 * Device information structure (moved from audio_device_interface.hh)
 */
struct device_info_v2 {
    std::string name;
    std::string id;
    bool is_default;
    channels_t channels;
    sample_rate_t sample_rate;
};

/**
 * Unified audio backend interface combining initialization and device management.
 * This interface merges the functionality of audio_backend and audio_device_interface
 * into a single, cohesive interface.
 * 
 * @note This is the v2 interface. The original audio_backend interface is preserved
 *       for backward compatibility during the migration period.
 */
class audio_backend {
public:
    virtual ~audio_backend() = default;
    
    // ========================================================================
    // Initialization and lifecycle management
    // ========================================================================
    
    /**
     * Initialize the audio subsystem.
     * This should be called once before any other backend operations.
     * @throws std::runtime_error if initialization fails
     */
    virtual void init() = 0;
    
    /**
     * Shutdown the audio subsystem.
     * This should clean up all resources and close all open devices.
     */
    virtual void shutdown() = 0;
    
    /**
     * Get the name of this backend.
     * @return Backend name (e.g., "SDL3", "ALSA", "PulseAudio", "Null")
     */
    virtual std::string get_name() const = 0;
    
    /**
     * Check if the backend is initialized.
     * @return true if initialized, false otherwise
     */
    virtual bool is_initialized() const = 0;
    
    // ========================================================================
    // Device enumeration and discovery
    // ========================================================================
    
    /**
     * Enumerate available audio devices.
     * @param playback If true, enumerate playback devices; if false, recording devices
     * @return Vector of available devices
     */
    virtual std::vector<device_info_v2> enumerate_devices(bool playback) = 0;
    
    /**
     * Get the default device.
     * @param playback If true, get default playback device; if false, default recording device
     * @return Default device info
     */
    virtual device_info_v2 get_default_device(bool playback) = 0;
    
    // ========================================================================
    // Device management
    // ========================================================================
    
    /**
     * Open an audio device.
     * @param device_id Device identifier (empty string for default)
     * @param spec Desired audio specification
     * @param obtained_spec Actual specification obtained (output parameter)
     * @return Device handle for use in subsequent operations
     * @throws std::runtime_error if device cannot be opened
     */
    virtual uint32_t open_device(const std::string& device_id, 
                                 const audio_spec& spec, 
                                 audio_spec& obtained_spec) = 0;
    
    /**
     * Close an audio device.
     * @param device_handle Device handle returned by open_device
     */
    virtual void close_device(uint32_t device_handle) = 0;
    
    // ========================================================================
    // Device properties
    // ========================================================================
    
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
    virtual sample_rate_t get_device_frequency(uint32_t device_handle) = 0;
    
    /**
     * Get device channel count.
     * @param device_handle Device handle
     * @return Number of channels
     */
    virtual channels_t get_device_channels(uint32_t device_handle) = 0;
    
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
    
    // ========================================================================
    // Device control
    // ========================================================================
    
    /**
     * Pause audio playback for a device.
     * @param device_handle Device handle
     * @return true on success, false on failure
     */
    virtual bool pause_device(uint32_t device_handle) = 0;
    
    /**
     * Resume audio playback for a device.
     * @param device_handle Device handle
     * @return true on success, false on failure
     */
    virtual bool resume_device(uint32_t device_handle) = 0;
    
    /**
     * Check if device is paused.
     * @param device_handle Device handle
     * @return true if paused, false otherwise
     */
    virtual bool is_device_paused(uint32_t device_handle) = 0;
    
    // ========================================================================
    // Stream creation
    // ========================================================================
    
    /**
     * Create an audio stream for the device.
     * Streams are the primary mechanism for sending audio data to a device.
     * 
     * @param device_handle Device handle returned by open_device
     * @param spec Audio specification for the stream
     * @param callback Optional callback function for audio data processing
     * @param userdata Optional user data passed to the callback
     * @return Audio stream interface for data operations
     * @throws std::runtime_error if stream cannot be created
     */
    virtual std::unique_ptr<audio_stream_interface> create_stream(
        uint32_t device_handle,
        const audio_spec& spec,
        void (*callback)(void* userdata, uint8_t* stream, int len),
        void* userdata
    ) = 0;
    
    // ========================================================================
    // Backend capabilities
    // ========================================================================
    
    /**
     * Check if this backend supports recording devices.
     * @return true if recording devices are supported, false otherwise
     */
    virtual bool supports_recording() const = 0;
    
    /**
     * Get maximum number of simultaneous open devices.
     * @return Maximum device count, or -1 for unlimited
     */
    virtual int get_max_open_devices() const = 0;
    
    // ========================================================================
    // Convenience wrappers (non-virtual) for common use cases
    // ========================================================================
    
    /**
     * Enumerate playback devices (convenience wrapper).
     * @return Vector of available playback devices
     */
    std::vector<device_info_v2> enumerate_playback_devices() {
        return enumerate_devices(true);
    }
    
    /**
     * Enumerate recording devices (convenience wrapper).
     * @return Vector of available recording devices
     */
    std::vector<device_info_v2> enumerate_recording_devices() {
        return enumerate_devices(false);
    }
    
    /**
     * Get the default playback device (convenience wrapper).
     * @return Default playback device info
     */
    device_info_v2 get_default_playback_device() {
        return get_default_device(true);
    }
    
    /**
     * Get the default recording device (convenience wrapper).
     * @return Default recording device info
     */
    device_info_v2 get_default_recording_device() {
        return get_default_device(false);
    }
    
    /**
     * Create a stream without callback (convenience wrapper).
     * @param device_handle Device handle
     * @param spec Audio specification
     * @return Audio stream interface
     */
    std::unique_ptr<audio_stream_interface> create_stream(
        uint32_t device_handle,
        const audio_spec& spec) {
        return create_stream(device_handle, spec, nullptr, nullptr);
    }
};

} // namespace musac

#endif // MUSAC_SDK_AUDIO_BACKEND_V2_HH