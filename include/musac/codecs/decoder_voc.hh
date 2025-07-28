//
// Created by igor on 3/18/25.
//

#ifndef  DECODER_VOC_HH
#define  DECODER_VOC_HH

#include <musac/sdk/proc_decoder.hh>

namespace musac {
    class decoder_voc : public proc_decoder {
        public:
            decoder_voc();
            ~decoder_voc() override;
    };
}

#endif
