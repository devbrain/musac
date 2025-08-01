//
// Created by igor on 3/18/25.
//

#ifndef  SAMPLES_CONVERTER_HH
#define  SAMPLES_CONVERTER_HH

#include <SDL3/SDL.h>
#include <musac/sdk/export_musac_sdk.h>

namespace musac {
    using to_float_converter_func_t = void (*)(float dst[], const Uint8* buff, unsigned int samples);
    MUSAC_SDK_EXPORT to_float_converter_func_t get_to_float_conveter(SDL_AudioFormat format);

    MUSAC_SDK_EXPORT int bytes_per_sample(SDL_AudioFormat format);
}





#endif
