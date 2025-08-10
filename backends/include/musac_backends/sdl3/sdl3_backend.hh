#ifndef MUSAC_BACKENDS_SDL3_BACKEND_HH
#define MUSAC_BACKENDS_SDL3_BACKEND_HH

#include <memory>

// Export macro for shared library builds
#ifdef MUSAC_BACKEND_SDL3_SHARED
    #ifdef _WIN32
        #ifdef MUSAC_BACKEND_SDL3_EXPORTS
            #define MUSAC_BACKEND_SDL3_EXPORT __declspec(dllexport)
        #else
            #define MUSAC_BACKEND_SDL3_EXPORT __declspec(dllimport)
        #endif
    #else
        #define MUSAC_BACKEND_SDL3_EXPORT __attribute__((visibility("default")))
    #endif
#else
    #define MUSAC_BACKEND_SDL3_EXPORT
#endif

// Public factory header for SDL3 backend
// This header can be included by applications that want to use SDL3 backend

namespace musac {

// Forward declaration
class audio_backend;

/**
 * Create an SDL3 audio backend instance.
 * 
 * The SDL3 backend provides:
 * - Full audio device enumeration and management
 * - Support for device switching during playback
 * - High-performance audio streaming
 * - Wide platform support (Windows, Linux, macOS, etc.)
 * 
 * @return New SDL3 backend instance
 * @throws std::runtime_error if SDL3 is not available
 * 
 * @note The backend must be initialized by calling init() before use
 * 
 * Example usage:
 * @code
 * auto backend = musac::create_sdl3_backend_v2();
 * backend->init();
 * auto devices = backend->enumerate_playback_devices();
 * @endcode
 */
MUSAC_BACKEND_SDL3_EXPORT std::unique_ptr<audio_backend> create_sdl3_backend();

} // namespace musac

#endif // MUSAC_BACKENDS_SDL3_BACKEND_HH