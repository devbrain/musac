#ifndef MUSAC_AUDIO_BACKEND_HH
#define MUSAC_AUDIO_BACKEND_HH

#include <string>
#include <memory>
#include <musac/export_musac.h>

namespace musac {

/**
 * Abstract interface for audio system initialization.
 * Implementations handle platform-specific audio subsystem setup.
 */
class MUSAC_EXPORT audio_backend {
public:
    virtual ~audio_backend() = default;
    
    /**
     * Initialize the audio subsystem.
     * @return true on success, false on failure
     */
    virtual bool init() = 0;
    
    /**
     * Shutdown the audio subsystem.
     */
    virtual void shutdown() = 0;
    
    /**
     * Get the name of this backend.
     * @return Backend name (e.g., "SDL3", "ALSA", "Null")
     */
    virtual std::string get_name() const = 0;
    
    /**
     * Check if the backend is initialized.
     * @return true if initialized, false otherwise
     */
    virtual bool is_initialized() const = 0;
};

/**
 * Factory function to create the default audio backend.
 * This can be configured at build time.
 */
MUSAC_EXPORT std::unique_ptr<audio_backend> create_default_audio_backend();

} // namespace musac

#endif // MUSAC_AUDIO_BACKEND_HH