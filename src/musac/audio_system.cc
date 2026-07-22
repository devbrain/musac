//
// Created by igor on 3/17/25.
//

#include <musac/audio_system.hh>
#include <musac/sdk/audio_backend.hh>
#include <musac/sdk/decoders_registry.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include "audio_mixer.hh"
#include "stream_container.hh"
#include <memory>
#include <mutex>
#include <vector>

namespace musac {

    extern void close_audio_devices();
    extern void close_audio_stream();
    extern void reset_audio_stream();
    extern audio_device* get_active_audio_device();
    
    // Immortal audio-system state. Function-local statics so they initialise on first use
    // (no static-INITIALISATION-order fiasco -- a client may call init() during its own static
    // construction) and are never torn down: the holders are heap-allocated and deliberately
    // leaked, so a client's audio_system::done() called from a LATE destructor still finds
    // valid state instead of resetting already-destroyed shared_ptrs and double-freeing the
    // backend / registry (a static-DESTRUCTION-order fiasco).
    static std::shared_ptr<audio_backend>& s_backend_v2() {
        static auto* p = new std::shared_ptr<audio_backend>();
        return *p;
    }
    static std::shared_ptr<decoders_registry>& s_decoders_registry() {
        static auto* p = new std::shared_ptr<decoders_registry>();
        return *p;
    }
    static std::mutex& s_system_mutex() {
        static auto* m = new std::mutex();
        return *m;
    }
    
    
    // Helper structures for device switching
    struct stream_playback_state {
        audio_stream* stream;
        bool was_playing;
        bool was_paused;
    };
    

    // New v2 API init method
    bool audio_system::init(std::shared_ptr<audio_backend> backend) {
        std::lock_guard<std::mutex> lock(s_system_mutex());
        
        if (!backend) {
            return false;
        }
        
        // Store the v2 backend
        s_backend_v2() = backend;
        
        // Initialize if not already initialized
        if (!backend->is_initialized()) {
            try {
                backend->init();
            } catch (const std::exception& e) {
                s_backend_v2().reset();
                return false;
            }
        }
        
        return true;
    }
    
    // Init with backend and registry
    bool audio_system::init(std::shared_ptr<audio_backend> backend, 
                           std::shared_ptr<decoders_registry> registry) {
        std::lock_guard<std::mutex> lock(s_system_mutex());
        
        if (!backend) {
            return false;
        }
        
        // Store the v2 backend and registry
        s_backend_v2() = backend;
        s_decoders_registry() = registry;
        
        // Initialize if not already initialized
        if (!backend->is_initialized()) {
            try {
                backend->init();
            } catch (const std::exception& e) {
                s_backend_v2().reset();
                s_decoders_registry().reset();
                return false;
            }
        }
        
        return true;
    }
    

    std::shared_ptr<audio_backend> audio_system::get_backend() {
        std::lock_guard<std::mutex> lock(s_system_mutex());
        return s_backend_v2();
    }
    
    std::shared_ptr<const decoders_registry> audio_system::get_decoders_registry() {
        // Shared ownership: a raw pointer could dangle if a concurrent
        // set_decoders_registry()/done() released the registry while the
        // caller was still using it.
        std::lock_guard<std::mutex> lock(s_system_mutex());
        return s_decoders_registry();
    }
    
    void audio_system::set_decoders_registry(std::shared_ptr<decoders_registry> registry) {
        std::lock_guard<std::mutex> lock(s_system_mutex());
        s_decoders_registry() = registry;
    }
    
    void audio_system::done() {
        close_audio_stream();
        close_audio_devices();
        
        // Clear the registry
        s_decoders_registry().reset();
        
        // Shutdown v2 backend if present
        if (s_backend_v2()) {
            if (s_backend_v2()->is_initialized()) {
                s_backend_v2()->shutdown();
            }
            s_backend_v2().reset();
        }
    }
    
    
    bool audio_system::switch_device(audio_device& new_device) {
        std::lock_guard<std::mutex> lock(s_system_mutex());

        if (!s_backend_v2()) {
            return false;
        }

        // Freeze the audio callback and stream destruction/moves for the whole
        // switch: the raw audio_stream* collected in stream_states below are
        // dereferenced across device teardown, which is only safe while
        // destructors are blocked on this mutex.
        std::lock_guard<std::recursive_mutex> cb_lock(audio_callback_mutex());

        try {
            // 1. Get current device (if any)
            audio_device* current_device = get_active_audio_device();
            if (!current_device) {
                // No active device, just make the new device active
                // The new device is already registered as active from its constructor
                LOG_INFO("audio_system", "No previous device, new device is now active");
                return true;
            }
            
            // 2. Check if it's the same device
            if (current_device == &new_device) {
                LOG_INFO("audio_system", "New device is the same as current device");
                return true;
            }
            
            // 3. Check if format conversion is needed
            bool format_conversion_needed = 
                (current_device->get_format() != new_device.get_format() ||
                 current_device->get_channels() != new_device.get_channels() ||
                 current_device->get_freq() != new_device.get_freq());
            
            if (format_conversion_needed) {
                LOG_INFO("audio_system", "Format conversion will be performed during device switch");
                LOG_INFO("audio_system", "Current: ", current_device->get_freq(), " Hz, ", 
                         static_cast<int>(current_device->get_channels()), " ch, format ",
                         current_device->get_format());
                LOG_INFO("audio_system", "New: ", new_device.get_freq(), " Hz, ", 
                         static_cast<int>(new_device.get_channels()), " ch, format ",
                         new_device.get_format());
            }
            
            // 4. Capture mixer state
            auto& mixer = audio_stream::get_global_mixer();
            auto mixer_snapshot = mixer.capture_state();
            
            // 5. Pause all streams and track their state
            std::vector<stream_playback_state> stream_states;
            auto streams = mixer.get_streams();
            
            if (streams) {
                for (const auto& entry : *streams) {
                    // Pin instead of a racy expired() check; combined with the
                    // callback mutex above the pointer can't be freed while we
                    // use it (see audio_mixer::capture_state).
                    auto pin = entry.lifetime_token.lock();
                    if (entry.stream && pin) {
                        stream_playback_state state;
                        state.stream = entry.stream;
                        state.was_playing = entry.stream->is_playing();
                        state.was_paused = entry.stream->is_paused();
                        
                        // Only pause streams that are currently playing
                        if (state.was_playing && !state.was_paused) {
                            entry.stream->pause();
                        }
                        
                        stream_states.push_back(state);
                    }
                }
            }
            
            // 6. Close the audio stream to stop callbacks
            close_audio_stream();
            
            // 7. The new device constructor already set it as active
            // We just need to restore the mixer state
            
            // 8. If format conversion is needed, re-open all streams with new device specifications
            if (format_conversion_needed) {
                LOG_INFO("audio_system", "Re-opening streams with new device format");
                
                // Force all streams to re-open with the new device's audio specs
                for (const auto& state : stream_states) {
                    try {
                        // The stream will pick up the new device specs when it opens
                        state.stream->open();
                    } catch (const std::exception& e) {
                        LOG_ERROR("audio_system", "Failed to re-open stream: %s", e.what());
                        // Continue with other streams even if one fails
                    }
                }
            }
            
            // 9. Restore mixer state
            mixer.restore_state(mixer_snapshot);
            
            // 10. Resume previously playing streams
            for (const auto& state : stream_states) {
                if (state.was_playing && !state.was_paused) {
                    state.stream->resume();
                }
            }
            
            LOG_INFO("audio_system", "Successfully switched to new device");
            return true;
            
        } catch (const std::exception& e) {
            // Error during switch - log and return false
            LOG_ERROR("audio_system", "Device switch failed: %s", e.what());
            return false;
        }
    }
}
