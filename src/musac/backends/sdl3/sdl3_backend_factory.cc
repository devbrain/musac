#include <musac/backends/sdl3/sdl3_backend.hh>
#include "sdl3_backend.hh"

namespace musac {

std::unique_ptr<audio_backend> create_sdl3_backend() {
    return std::make_unique<sdl3_backend>();
}

} // namespace musac