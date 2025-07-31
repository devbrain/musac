//
// Created by igor on 3/18/25.
//

#include <musac/codecs/decoder_aiff.hh>

#include "musac/codecs/aiff/load_aiff.h"

namespace musac {
    decoder_aiff::decoder_aiff()
        : proc_decoder(Mix_LoadAIFF_IO) {
    }

    decoder_aiff::~decoder_aiff() = default;
}
