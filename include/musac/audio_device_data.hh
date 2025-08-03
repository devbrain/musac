//
// Created by igor on 4/28/25.
//

#pragma once
#include <memory>
#include <musac/sdk/from_float_converter.hh>
#include <musac/sdk/audio_format.h>

namespace musac {
    class audio_stream_interface;
    
    struct audio_device_data {
        audio_spec m_audio_spec;
        std::shared_ptr<audio_stream_interface> m_stream;
        int m_frame_size;
        // This points to an appropriate converter for the current audio format.
        from_float_converter_func_t  m_sample_converter;
    };
}


