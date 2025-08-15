/**
 * @file audio_system.hh
 * @brief Global audio system management
 * @ingroup core
 */

#ifndef  MUSAC_AUDIO_SYSTEM_HH
#define  MUSAC_AUDIO_SYSTEM_HH

#include <musac/export_musac.h>
#include <memory>

namespace musac {
    // Forward declarations
    class audio_device;
    class audio_backend;
    class decoders_registry;
    
    /**
     * @struct audio_system
     * @brief Global audio system initialization and management
     * @ingroup core
     * 
     * The audio_system provides system-wide initialization and configuration
     * for the musac library. It manages the audio backend selection and
     * decoder registry.
     * 
     * ## Usage Pattern
     * 
     * @code
     * // Initialize with SDL3 backend
     * auto backend = std::make_shared<sdl3_backend>();
     * if (!musac::audio_system::init(backend)) {
     *     // Handle initialization failure
     * }
     * 
     * // Optionally set custom decoders
     * auto registry = std::make_shared<decoders_registry>();
     * registry->register_decoder<mp3_decoder>();
     * registry->register_decoder<flac_decoder>();
     * musac::audio_system::set_decoders_registry(registry);
     * 
     * // Use audio devices...
     * musac::audio_device device;
     * device.open_default_device();
     * 
     * // Cleanup when done
     * musac::audio_system::done();
     * @endcode
     * 
     * ## Backend Selection
     * 
     * The audio system requires a backend to interface with the platform's
     * audio subsystem. Available backends:
     * 
     * - **SDL3**: Cross-platform via SDL3 (recommended)
     * - **SDL2**: Cross-platform via SDL2 (legacy)
     * - **Null**: Silent backend for testing
     * - **Custom**: User-provided backend implementation
     * 
     * ## Decoder Registry
     * 
     * The decoder registry manages available audio format decoders.
     * If not explicitly set, a default registry with all built-in
     * decoders is used.
     * 
     * ## Thread Safety
     * 
     * - init() and done() are not thread-safe, call from main thread
     * - get_backend() and get_decoders_registry() are thread-safe
     * - switch_device() handles thread synchronization internally
     * 
     * @warning Call done() before program exit to ensure proper cleanup
     * @see audio_backend, decoders_registry, audio_device
     */
    struct MUSAC_EXPORT audio_system {
        /**
         * @brief Initialize audio system with a backend
         * @param backend Audio backend implementation
         * @return true if initialization succeeded
         * 
         * Initializes the global audio system with the specified backend.
         * This must be called before creating any audio devices.
         * 
         * @code
         * auto backend = std::make_shared<sdl3_backend>();
         * if (!audio_system::init(backend)) {
         *     std::cerr << "Failed to initialize audio\n";
         * }
         * @endcode
         * 
         * @note Only call once at program startup
         */
        static bool init(std::shared_ptr<audio_backend> backend);
        
        /**
         * @brief Initialize with backend and custom decoder registry
         * @param backend Audio backend implementation
         * @param registry Custom decoder registry
         * @return true if initialization succeeded
         * 
         * Initializes the audio system with both a backend and a custom
         * decoder registry. Use this to limit available decoders or add
         * custom format support.
         * 
         * @code
         * auto backend = std::make_shared<sdl3_backend>();
         * auto registry = std::make_shared<decoders_registry>();
         * registry->register_decoder<my_custom_decoder>();
         * 
         * if (!audio_system::init(backend, registry)) {
         *     std::cerr << "Failed to initialize audio\n";
         * }
         * @endcode
         */
        static bool init(std::shared_ptr<audio_backend> backend, 
                        std::shared_ptr<decoders_registry> registry);

        /**
         * @brief Shutdown the audio system
         * 
         * Cleans up all audio resources and shuts down the backend.
         * Call this before program exit to ensure proper cleanup.
         * 
         * @warning All audio devices and streams must be destroyed before calling
         */
        static void done();
        
        /**
         * @brief Get the current audio backend
         * @return Shared pointer to backend, or nullptr if not initialized
         * 
         * @note Thread-safe
         */
        static std::shared_ptr<audio_backend> get_backend();
        
        /**
         * @brief Get the decoder registry
         * @return Pointer to registry, or nullptr if not set
         * 
         * Returns the current decoder registry used for format detection
         * and decoding. If no custom registry was set, returns the default
         * registry with all built-in decoders.
         * 
         * @note Thread-safe
         */
        static const decoders_registry* get_decoders_registry();
        
        /**
         * @brief Set a custom decoder registry
         * @param registry New decoder registry
         * 
         * Replaces the current decoder registry. Can be called after init()
         * to change available decoders at runtime.
         * 
         * @code
         * // Add a new decoder at runtime
         * auto registry = std::make_shared<decoders_registry>(*audio_system::get_decoders_registry());
         * registry->register_decoder<new_format_decoder>();
         * audio_system::set_decoders_registry(registry);
         * @endcode
         */
        static void set_decoders_registry(std::shared_ptr<decoders_registry> registry);
        
        /**
         * @brief Switch to a different audio device
         * @param new_device Pre-configured audio device
         * @return true if switch succeeded
         * 
         * Seamlessly switches all active streams to a new audio device.
         * Stream positions and states are preserved during the switch.
         * 
         * @code
         * audio_device new_device;
         * new_device.open_device(other_device_id);
         * 
         * if (audio_system::switch_device(new_device)) {
         *     std::cout << "Switched to new device\n";
         * }
         * @endcode
         * 
         * @note Handles all thread synchronization internally
         */
        static bool switch_device(audio_device& new_device);
    };
}



#endif
