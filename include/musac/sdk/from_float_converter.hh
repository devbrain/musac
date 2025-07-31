//
// Created by igor on 4/28/25.
//

#ifndef  FROM_FLOAT_CONVERTER_HH
#define  FROM_FLOAT_CONVERTER_HH

#include <musac/sdk/buffer.hh>
#include <SDL3/SDL.h>

#include <musac/sdk/export_musac_sdk.h>

namespace musac {
    using from_float_converter_func_t = void(*)(Uint8* dst, size_t dstBytes, const buffer<float>& src);
    MUSAC_SDK_EXPORT from_float_converter_func_t get_from_float_converter(SDL_AudioFormat fmt);
}



#endif
