#include <musac/audio_backend.hh>
#include <musac/audio_device_interface.hh>

#ifdef MUSAC_HAS_SDL3_BACKEND
#include "backends/sdl3/sdl3_audio_backend.hh"
#include "backends/sdl3/sdl3_device_manager.hh"
#endif

#ifdef MUSAC_HAS_NULL_BACKEND
#include "backends/null/null_audio_backend.hh"
#include "backends/null/null_device_manager.hh"
#endif

namespace musac {

std::unique_ptr<audio_backend> create_default_audio_backend() {
#ifdef MUSAC_HAS_SDL3_BACKEND
    return std::make_unique<sdl3_audio_backend>();
#elif defined(MUSAC_HAS_NULL_BACKEND)
    return std::make_unique<null_audio_backend>();
#else
    // No backend available
    return nullptr;
#endif
}

std::unique_ptr<audio_device_interface> create_default_audio_device_manager() {
#ifdef MUSAC_HAS_SDL3_BACKEND
    return std::make_unique<sdl3_device_manager>();
#elif defined(MUSAC_HAS_NULL_BACKEND)
    return std::make_unique<null_device_manager>();
#else
    // No device manager available
    return nullptr;
#endif
}

} // namespace musac