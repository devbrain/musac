#include <musac_backends/sdl2/sdl2_backend.hh>
#include "sdl2_backend_impl.hh"

namespace musac {

std::unique_ptr<audio_backend> create_sdl2_backend() {
    return std::make_unique<sdl2_backend>();
}

} // namespace musac