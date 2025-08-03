#include "sdl3_audio_backend.hh"
#include <SDL3/SDL.h>

namespace musac {

sdl3_audio_backend::sdl3_audio_backend() : m_initialized(false) {}

sdl3_audio_backend::~sdl3_audio_backend() {
    if (m_initialized) {
        shutdown();
    }
}

bool sdl3_audio_backend::init() {
    if (m_initialized) {
        return true;
    }
    
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void sdl3_audio_backend::shutdown() {
    if (m_initialized) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        m_initialized = false;
    }
}

std::string sdl3_audio_backend::get_name() const {
    return "SDL3";
}

bool sdl3_audio_backend::is_initialized() const {
    return m_initialized;
}

} // namespace musac