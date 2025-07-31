//
// Created by igor on 3/18/25.
//

#ifndef  DECODER_AIFF_HH
#define  DECODER_AIFF_HH

#include <musac/sdk/proc_decoder.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    class MUSAC_CODECS_EXPORT decoder_aiff : public proc_decoder {
        public:
            decoder_aiff();
            ~decoder_aiff() override;
    };
}

#endif
