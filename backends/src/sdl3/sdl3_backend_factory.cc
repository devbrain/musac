/**
 * @file sdl3_backend_factory.cc
 * @brief SDL3 backend factory implementation
 * @ingroup sdl3_backend
 */

#include <musac_backends/sdl3/sdl3_backend.hh>
#include "sdl3_backend_impl.hh"

namespace musac {

/**
 * @brief Factory function implementation for SDL3 backend
 * 
 * This factory function creates instances of the SDL3 backend without
 * exposing the implementation details. The returned backend must be
 * initialized before use.
 * 
 * @note The factory pattern allows the backend implementation to remain
 *       private while providing a public interface for creation.
 */
std::unique_ptr<audio_backend> create_sdl3_backend() {
    return std::make_unique<sdl3_backend>();
}

} // namespace musac