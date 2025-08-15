/**
 * @file sdl2_backend.hh
 * @brief SDL2 audio backend factory
 * @ingroup backends
 */

#ifndef MUSAC_BACKENDS_SDL2_BACKEND_HH
#define MUSAC_BACKENDS_SDL2_BACKEND_HH

#include <memory>


#include "export_musac_backend_sdl2.h"

namespace musac {

/**
 * @defgroup sdl2_backend SDL2 Audio Backend
 * @ingroup backends
 * @brief SDL2-based audio backend for cross-platform audio support
 * 
 * The SDL2 backend provides comprehensive audio functionality using the
 * Simple DirectMedia Layer 2 library. It offers excellent cross-platform
 * support and is ideal for game development and multimedia applications.
 * 
 * ## Features
 * 
 * - **Cross-platform**: Works on Windows, Linux, macOS, iOS, Android
 * - **Device enumeration**: List all available audio devices
 * - **Hot-plug support**: Detect device connection/disconnection
 * - **Low latency**: Optimized for real-time audio
 * - **Format flexibility**: Supports various audio formats
 * - **Thread-safe**: Safe for multi-threaded applications
 * 
 * ## Platform Support
 * 
 * | Platform | Audio System | Status |
 * |----------|-------------|--------|
 * | Windows  | WASAPI, DirectSound | ✅ Full support |
 * | Linux    | ALSA, PulseAudio, JACK | ✅ Full support |
 * | macOS    | CoreAudio | ✅ Full support |
 * | iOS      | CoreAudio | ✅ Full support |
 * | Android  | OpenSL ES | ✅ Full support |
 * | FreeBSD  | OSS | ✅ Full support |
 * 
 * ## Usage Example
 * 
 * @code
 * #include <musac_backends/sdl2/sdl2_backend.hh>
 * #include <musac/audio_device.hh>
 * 
 * // Create and register SDL2 backend
 * auto backend = musac::create_sdl2_backend();
 * 
 * // Initialize the backend
 * backend->init();
 * 
 * // Use with audio device
 * musac::audio_device device;
 * device.set_backend(std::move(backend));
 * device.open_default_device();
 * 
 * // Create and play audio stream
 * auto source = musac::load_wav("music.wav");
 * auto stream = device.create_stream(std::move(source));
 * stream->play();
 * @endcode
 * 
 * ## Performance Characteristics
 * 
 * - **Latency**: 10-20ms typical (configurable)
 * - **CPU usage**: < 1% for stereo 48kHz playback
 * - **Memory**: ~100KB per device + buffer memory
 * 
 * ## Configuration
 * 
 * SDL2 backend respects SDL2 audio environment variables:
 * - `SDL_AUDIODRIVER`: Force specific audio driver
 * - `SDL_AUDIO_FREQUENCY`: Default sample rate
 * - `SDL_AUDIO_CHANNELS`: Default channel count
 * - `SDL_AUDIO_SAMPLES`: Buffer size in samples
 * 
 * ## Limitations
 * 
 * - Requires SDL2 2.0.4 or later
 * - Maximum 8 channels per device
 * - Sample rates limited to common values (8kHz-192kHz)
 * 
 * @{
 */

// Forward declaration
class audio_backend;

/**
 * @brief Create an SDL2 audio backend instance
 * @return New SDL2 backend instance
 * @throws std::runtime_error if SDL2 library is not available
 * 
 * Creates a new SDL2 backend instance. The backend must be initialized
 * by calling init() before use. This function does not initialize SDL2
 * itself, allowing for custom SDL2 initialization if needed.
 * 
 * ## Thread Safety
 * 
 * The returned backend is thread-safe after initialization. Multiple
 * threads can safely call backend methods, though device operations
 * may be serialized internally.
 * 
 * ## Resource Management
 * 
 * The backend automatically cleans up SDL2 audio subsystem when destroyed.
 * Ensure all streams and devices are closed before destroying the backend.
 * 
 * ## Example: Custom Configuration
 * 
 * @code
 * // Set SDL2 audio driver before creating backend
 * #ifdef _WIN32
 *     setenv("SDL_AUDIODRIVER", "wasapi", 1);
 * #elif __linux__
 *     setenv("SDL_AUDIODRIVER", "pulse", 1);
 * #endif
 * 
 * auto backend = musac::create_sdl2_backend();
 * backend->init();
 * 
 * // Check which driver was actually used
 * std::cout << "Using backend: " << backend->get_name() << '\n';
 * @endcode
 * 
 * @see audio_backend, create_sdl3_backend()
 */
MUSAC_BACKEND_SDL2_EXPORT std::unique_ptr<audio_backend> create_sdl2_backend();

/** @} */ // end of sdl2_backend group

} // namespace musac

#endif // MUSAC_BACKENDS_SDL2_BACKEND_HH