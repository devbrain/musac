/**
 * @file sdl2_backend_factory.cc
 * @brief SDL2 backend factory implementation
 * @ingroup sdl2_backend
 */

#include <musac_backends/sdl2/sdl2_backend.hh>
#include "sdl2_backend_impl.hh"

namespace musac {

/**
 * @brief Factory function implementation for SDL2 backend
 * 
 * This factory function creates instances of the SDL2 backend without
 * exposing the implementation details. The returned backend must be
 * initialized before use.
 * 
 * @note The factory pattern allows the backend implementation to remain
 *       private while providing a public interface for creation.
 */
std::unique_ptr<audio_backend> create_sdl2_backend() {
    return std::make_unique<sdl2_backend>();
}

} // namespace musac