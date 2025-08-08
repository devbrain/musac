//
// Created by igor on 3/18/25.
//

#ifndef  SAMPLES_CONVERTER_HH
#define  SAMPLES_CONVERTER_HH

#include <musac/sdk/audio_format.hh>
#include <musac/sdk/types.hh>
#include <musac/sdk/export_musac_sdk.h>

namespace musac {
    using to_float_converter_func_t = void (*)(float dst[], const uint8* buff, unsigned int samples);
    MUSAC_SDK_EXPORT to_float_converter_func_t get_to_float_conveter(audio_format format);

    MUSAC_SDK_EXPORT int bytes_per_sample(audio_format format);
}





#endif
