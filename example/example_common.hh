/**
 * @file example_common.hh
 * @brief Common utilities for musac examples
 * 
 * This header provides backend selection based on build configuration.
 */

#ifndef MUSAC_EXAMPLE_COMMON_HH
#define MUSAC_EXAMPLE_COMMON_HH

#include <iostream>
#include <memory>

// Forward declaration
namespace musac {
    class audio_backend;
}

#ifdef MUSAC_USE_SDL3_BACKEND
#include <musac_backends/sdl3/sdl3_backend.hh>
#elif defined(MUSAC_USE_SDL2_BACKEND)
#include <musac_backends/sdl2/sdl2_backend.hh>
#else
#error "No backend defined. Please enable SDL2 or SDL3 backend in CMake configuration."
#endif

namespace musac {
    namespace examples {
        /**
         * @brief Create the default backend based on build configuration
         * @return Shared pointer to the configured backend
         *
         * This function creates the appropriate backend based on compile-time
         * configuration. SDL3 is preferred if both backends are available.
         */
        inline std::shared_ptr <audio_backend> create_default_backend() {
#ifdef MUSAC_USE_SDL3_BACKEND
            std::cout << "Using SDL3 backend" << std::endl;
            return std::shared_ptr <audio_backend>(create_sdl3_backend());
#elif defined(MUSAC_USE_SDL2_BACKEND)
            std::cout << "Using SDL2 backend" << std::endl;
            return std::shared_ptr <audio_backend>(create_sdl2_backend());
#endif
        }

        /**
         * @brief Get the backend name for display
         * @return Name of the configured backend
         */
        inline const char* get_backend_name() {
#ifdef MUSAC_USE_SDL3_BACKEND
            return "SDL3";
#elif defined(MUSAC_USE_SDL2_BACKEND)
            return "SDL2";
#endif
        }
    } // namespace examples
} // namespace musac

#endif // MUSAC_EXAMPLE_COMMON_HH
