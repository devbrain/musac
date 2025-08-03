//
// Created by igor on 3/17/25.
//

#ifndef  MUSAC_AUDIO_SYSTEM_HH
#define  MUSAC_AUDIO_SYSTEM_HH

#include <musac/export_musac.h>

namespace musac {
    struct MUSAC_EXPORT audio_system {
        static bool init();

        static void done();

    };
}



#endif
