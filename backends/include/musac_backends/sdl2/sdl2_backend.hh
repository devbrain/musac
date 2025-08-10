#ifndef MUSAC_BACKENDS_SDL2_BACKEND_HH
#define MUSAC_BACKENDS_SDL2_BACKEND_HH

#include <memory>

// Export macro for shared library builds
#ifdef MUSAC_BACKEND_SDL2_SHARED
    #ifdef _WIN32
        #ifdef MUSAC_BACKEND_SDL2_EXPORTS
            #define MUSAC_BACKEND_SDL2_EXPORT __declspec(dllexport)
        #else
            #define MUSAC_BACKEND_SDL2_EXPORT __declspec(dllimport)
        #endif
    #else
        #define MUSAC_BACKEND_SDL2_EXPORT __attribute__((visibility("default")))
    #endif
#else
    #define MUSAC_BACKEND_SDL2_EXPORT
#endif

// Public factory header for SDL2 backend
// This header can be included by applications that want to use SDL2 backend

namespace musac {

// Forward declaration
class audio_backend;

/**
 * Create an SDL2 audio backend instance.
 * 
 * The SDL2 backend provides:
 * - Full audio device enumeration and management
 * - Support for device switching during playback
 * - High-performance audio streaming
 * - Wide platform support (Windows, Linux, macOS, etc.)
 * - Compatibility with SDL2-based applications
 * 
 * @return New SDL2 backend instance
 * @throws std::runtime_error if SDL2 is not available
 * 
 * @note The backend must be initialized by calling init() before use
 * 
 * Example usage:
 * @code
 * auto backend = musac::create_sdl2_backend();
 * backend->init();
 * auto devices = backend->enumerate_playback_devices();
 * @endcode
 */
MUSAC_BACKEND_SDL2_EXPORT std::unique_ptr<audio_backend> create_sdl2_backend();

} // namespace musac

#endif // MUSAC_BACKENDS_SDL2_BACKEND_HH