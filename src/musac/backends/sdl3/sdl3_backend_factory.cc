#include <musac/backends/sdl3/sdl3_backend.hh>
#include "sdl3_backend_v2.hh"

namespace musac {

std::unique_ptr<audio_backend_v2> create_sdl3_backend_v2() {
    return std::make_unique<sdl3_backend_v2>();
}

} // namespace musac