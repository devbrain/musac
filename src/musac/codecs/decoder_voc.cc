//
// Created by igor on 3/18/25.
//

#include <musac/codecs/decoder_voc.hh>
#include "musac/codecs/voc/load_voc.h"

namespace musac {
    decoder_voc::decoder_voc()
        : proc_decoder(Mix_LoadVOC_IO) {
    }

    decoder_voc::~decoder_voc() = default;
}
