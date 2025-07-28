//
// Created by igor on 3/17/25.
//

#include "../../include/musac/audio_system.hh"
#include <SDL3/SDL.h>
namespace musac {

    extern void close_audio_devices();
    extern void close_audio_stream();

    bool audio_system::init() {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            return false;
        }
        return true;
    }

    void audio_system::done() {
        close_audio_stream();
        close_audio_devices();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}
