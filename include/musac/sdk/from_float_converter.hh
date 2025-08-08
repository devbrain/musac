//
// Created by igor on 4/28/25.
//

#ifndef  FROM_FLOAT_CONVERTER_HH
#define  FROM_FLOAT_CONVERTER_HH

#include <musac/sdk/buffer.hh>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/types.hh>
#include <musac/sdk/export_musac_sdk.h>

namespace musac {
    using from_float_converter_func_t = void(*)(uint8* dst, size_t dstBytes, const buffer<float>& src);
    MUSAC_SDK_EXPORT from_float_converter_func_t get_from_float_converter(audio_format fmt);
}



#endif
