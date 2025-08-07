//
// Created by igor on 3/17/25.
//

#ifndef  MUSAC_AUDIO_SYSTEM_HH
#define  MUSAC_AUDIO_SYSTEM_HH

#include <musac/export_musac.h>
#include <memory>

namespace musac {
    // Forward declarations
    class audio_device;
    class audio_backend_v2;
    
    struct MUSAC_EXPORT audio_system {
        // Initialize with a backend (v2 API)
        static bool init(std::shared_ptr<audio_backend_v2> backend);

        static void done();
        
        // Get the current backend
        static std::shared_ptr<audio_backend_v2> get_backend();
        
        // Device switching - takes a user-created device
        static bool switch_device(audio_device& new_device);
    };
}



#endif
