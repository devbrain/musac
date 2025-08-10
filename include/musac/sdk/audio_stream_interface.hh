#ifndef MUSAC_AUDIO_STREAM_INTERFACE_HH
#define MUSAC_AUDIO_STREAM_INTERFACE_HH

#include <cstddef>
#include <musac/sdk/export_musac_sdk.h>
#include <cstdint>

namespace musac {

/**
 * Abstract interface for audio streaming.
 * Implementations handle platform-specific audio streaming operations.
 */
class MUSAC_SDK_EXPORT audio_stream_interface {
public:
    virtual ~audio_stream_interface() = default;
    
    /**
     * Put audio data into the stream.
     * @param data Audio data buffer
     * @param size Size of data in bytes
     * @return true on success, false on failure
     */
    virtual bool put_data(const void* data, size_t size) = 0;
    
    /**
     * Get audio data from the stream.
     * @param data Buffer to receive audio data
     * @param size Size of buffer in bytes
     * @return Number of bytes actually read
     */
    virtual size_t get_data(void* data, size_t size) = 0;
    
    /**
     * Clear any buffered data in the stream.
     */
    virtual void clear() = 0;
    
    /**
     * Pause the audio stream.
     * @return true on success, false on failure
     */
    virtual bool pause() = 0;
    
    /**
     * Resume the audio stream.
     * @return true on success, false on failure
     */
    virtual bool resume() = 0;
    
    /**
     * Check if the stream is paused.
     * @return true if paused, false otherwise
     */
    virtual bool is_paused() const = 0;
    
    /**
     * Get the number of bytes queued in the stream.
     * @return Number of bytes queued
     */
    virtual size_t get_queued_size() const = 0;
    
    /**
     * Bind this stream to a device for playback.
     * @return true on success, false on failure
     */
    virtual bool bind_to_device() = 0;
    
    /**
     * Unbind this stream from its device.
     */
    virtual void unbind_from_device() = 0;
};

} // namespace musac

#endif // MUSAC_AUDIO_STREAM_INTERFACE_HH