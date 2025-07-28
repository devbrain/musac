//
// Created by igor on 4/28/25.
//

#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include "sdk/from_float_converter.hh"

namespace musac {
    struct audio_device_data {
        SDL_AudioSpec m_audio_spec;
        std::shared_ptr <SDL_AudioStream> m_stream;
        int m_frame_size;
        // This points to an appropriate converter for the current audio format.
        from_float_converter_func_t  m_sample_converter;
    };
}


