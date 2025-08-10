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
    class audio_backend;
    class decoders_registry;
    
    struct MUSAC_EXPORT audio_system {
        // Initialize with a backend (v2 API)
        static bool init(std::shared_ptr<audio_backend> backend);
        
        // Initialize with backend and decoders registry
        static bool init(std::shared_ptr<audio_backend> backend, 
                        std::shared_ptr<decoders_registry> registry);

        static void done();
        
        // Get the current backend
        static std::shared_ptr<audio_backend> get_backend();
        
        // Get the decoders registry (may be nullptr if not set)
        static const decoders_registry* get_decoders_registry();
        
        // Set the decoders registry (can be called after init)
        static void set_decoders_registry(std::shared_ptr<decoders_registry> registry);
        
        // Device switching - takes a user-created device
        static bool switch_device(audio_device& new_device);
    };
}



#endif
