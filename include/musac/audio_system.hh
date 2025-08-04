//
// Created by igor on 3/17/25.
//

#ifndef  MUSAC_AUDIO_SYSTEM_HH
#define  MUSAC_AUDIO_SYSTEM_HH

#include <musac/export_musac.h>
#include <musac/audio_device_interface.hh>
#include <vector>

namespace musac {
    // Forward declaration
    class audio_device;
    
    struct MUSAC_EXPORT audio_system {
        static bool init();

        static void done();
        
        // Device enumeration
        static std::vector<device_info> enumerate_devices(bool playback_devices = true);
        static device_info get_default_device(bool playback_device = true);
        
        // Device switching - takes a user-created device
        static bool switch_device(audio_device& new_device);
    };
}



#endif
