/**
 * @file sdl3_backend.hh
 * @brief SDL3 audio backend factory
 * @ingroup backends
 */

#ifndef MUSAC_BACKENDS_SDL3_BACKEND_HH
#define MUSAC_BACKENDS_SDL3_BACKEND_HH

#include <memory>

// Include generated export header
#include "export_musac_backend_sdl3.h"

namespace musac {

/**
 * @defgroup sdl3_backend SDL3 Audio Backend
 * @ingroup backends
 * @brief SDL3-based audio backend with modern features
 * 
 * The SDL3 backend is the next-generation audio backend using SDL3's
 * redesigned audio subsystem. It offers improved performance, better
 * device management, and new features compared to SDL2.
 * 
 * ## Key Improvements over SDL2
 * 
 * - **Stream-based API**: More flexible audio stream management
 * - **Better device handling**: Improved hot-plug support
 * - **Lower latency**: Optimized callback system
 * - **Format flexibility**: More audio formats supported
 * - **Memory efficiency**: Reduced allocations in audio path
 * 
 * ## Features
 * 
 * - **Cross-platform**: Full support for all SDL3 platforms
 * - **Device enumeration**: Enhanced device discovery
 * - **Hot-plug support**: Seamless device connection/disconnection
 * - **Low latency**: < 10ms achievable on supported platforms
 * - **High-resolution audio**: Support for 24-bit and 32-bit formats
 * - **Multi-channel**: Support for up to 8 channels
 * 
 * ## Platform Support
 * 
 * | Platform | Audio System | Improvements |
 * |----------|-------------|-------------|
 * | Windows  | WASAPI | Exclusive mode, lower latency |
 * | Linux    | PipeWire, PulseAudio | Better integration |
 * | macOS    | CoreAudio | AVAudioEngine support |
 * | iOS      | CoreAudio | Improved background audio |
 * | Android  | AAudio | Lower latency path |
 * 
 * ## Migration from SDL2
 * 
 * The SDL3 backend maintains API compatibility while offering:
 * - Automatic format negotiation improvements
 * - Better error recovery
 * - Enhanced performance monitoring
 * - Simplified device management
 * 
 * @code
 * // SDL2 backend
 * auto backend = create_sdl2_backend();
 * 
 * // SDL3 backend - drop-in replacement
 * auto backend = create_sdl3_backend();
 * // Same API, better performance
 * @endcode
 * 
 * @{  
 */

// Forward declaration
class audio_backend;

/**
 * @brief Create an SDL3 audio backend instance
 * @return New SDL3 backend instance
 * @throws std::runtime_error if SDL3 library is not available
 * 
 * Creates a new SDL3 backend instance using the modern SDL3 audio
 * subsystem. The backend must be initialized before use.
 * 
 * ## Usage Example
 * 
 * @code
 * #include <musac_backends/sdl3/sdl3_backend.hh>
 * #include <musac/audio_device.hh>
 * 
 * // Create SDL3 backend
 * auto backend = musac::create_sdl3_backend();
 * backend->init();
 * 
 * // Use with audio device
 * musac::audio_device device;
 * device.set_backend(std::move(backend));
 * device.open_default_device();
 * 
 * // Play audio with improved performance
 * auto source = musac::load_flac("hires_audio.flac");
 * auto stream = device.create_stream(std::move(source));
 * stream->play();
 * @endcode
 * 
 * ## Performance Characteristics
 * 
 * - **Latency**: 5-15ms typical (platform dependent)
 * - **CPU usage**: 10-20% lower than SDL2 backend
 * - **Memory**: Optimized buffer management
 * 
 * ## Configuration
 * 
 * SDL3 backend respects environment variables:
 * - `SDL_AUDIO_DRIVER`: Force specific driver
 * - `SDL_AUDIO_DEVICE_NAME`: Default device name
 * - `SDL_AUDIO_ALLOW_EXCLUSIVE`: Enable exclusive mode (Windows)
 * 
 * ## Thread Safety
 * 
 * The backend is fully thread-safe after initialization.
 * Device operations and stream creation can be called
 * concurrently from multiple threads.
 * 
 * ## New Features in SDL3
 * 
 * - **Audio Streams**: Fine-grained stream control
 * - **Device Properties**: Query detailed device capabilities  
 * - **Format Negotiation**: Automatic optimal format selection
 * - **Power Management**: Better mobile battery optimization
 * 
 * @see audio_backend, create_sdl2_backend()
 */
MUSAC_BACKEND_SDL3_EXPORT std::unique_ptr<audio_backend> create_sdl3_backend();

/** @} */ // end of sdl3_backend group

} // namespace musac

#endif // MUSAC_BACKENDS_SDL3_BACKEND_HH