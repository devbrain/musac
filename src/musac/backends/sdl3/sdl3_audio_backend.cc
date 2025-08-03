#include "sdl3_audio_backend.hh"
#include <SDL3/SDL.h>
#include <failsafe/failsafe.hh>
#include <atomic>

namespace musac {

// Track SDL initialization globally
static std::atomic<int> s_sdl_init_count{0};

sdl3_audio_backend::sdl3_audio_backend() : m_initialized(false) {}

sdl3_audio_backend::~sdl3_audio_backend() {
    // Destructor should not call shutdown if already shut down
    if (m_initialized) {
        // LOG_INFO("SDL3Backend", "Destructor calling shutdown");
        shutdown();
    }
}

bool sdl3_audio_backend::init() {
    if (m_initialized) {
        return true;
    }
    
    // Increment SDL init count
    if (s_sdl_init_count++ == 0) {
        // First initialization - just call SDL_Init unconditionally
        SDL_Init(SDL_INIT_AUDIO);
        
        // Check if audio is now initialized
        if (!SDL_WasInit(SDL_INIT_AUDIO)) {
            s_sdl_init_count--;
            // LOG_ERROR("SDL3Backend", "Failed to initialize SDL audio subsystem");
            return false;
        }
        
        // LOG_INFO("SDL3Backend", "SDL audio subsystem initialized");
    } else {
        // LOG_INFO("SDL3Backend", "SDL already initialized, count:", s_sdl_init_count.load());
    }
    
    m_initialized = true;
    return true;
}

void sdl3_audio_backend::shutdown() {
    if (m_initialized) {
        m_initialized = false;
        
        // Decrement SDL init count
        if (--s_sdl_init_count == 0) {
            // Last shutdown - quit SDL entirely to ensure clean state
            SDL_Quit();
            // LOG_INFO("SDL3Backend", "SDL shut down completely");
        } else {
            // LOG_INFO("SDL3Backend", "SDL shutdown skipped, count:", s_sdl_init_count.load());
        }
    }
}

std::string sdl3_audio_backend::get_name() const {
    return "SDL3";
}

bool sdl3_audio_backend::is_initialized() const {
    return m_initialized;
}

} // namespace musac