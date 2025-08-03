//
// Created by igor on 3/17/25.
//

#include <musac/audio_system.hh>
#include <musac/audio_backend.hh>
#include <memory>
namespace musac {

    extern void close_audio_devices();
    extern void close_audio_stream();
    
    static std::unique_ptr<audio_backend> s_backend;

    bool audio_system::init() {
        if (!s_backend) {
            s_backend = create_default_audio_backend();
        }
        
        if (!s_backend) {
            return false;
        }
        
        return s_backend->init();
    }

    void audio_system::done() {
        close_audio_stream();
        close_audio_devices();
        
        if (s_backend) {
            s_backend->shutdown();
            s_backend.reset();
        }
    }
}
