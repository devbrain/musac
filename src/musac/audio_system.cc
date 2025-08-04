//
// Created by igor on 3/17/25.
//

#include <musac/audio_system.hh>
#include <musac/audio_backend.hh>
#include <musac/audio_device.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include <memory>
#include <mutex>

namespace musac {

    extern void close_audio_devices();
    extern void close_audio_stream();
    extern void reset_audio_stream();
    
    static std::unique_ptr<audio_backend> s_backend;
    static std::mutex s_system_mutex;

    bool audio_system::init() {
        if (!s_backend) {
            s_backend = create_default_audio_backend();
        }
        
        if (!s_backend) {
            return false;
        }
        
        try {
            s_backend->init();
            return true;
        } catch (const std::exception& e) {
            // Log the error and return false
            // The caller can decide whether to handle this as fatal
            s_backend.reset();
            return false;
        }
    }

    void audio_system::done() {
        close_audio_stream();
        close_audio_devices();
        
        if (s_backend) {
            s_backend->shutdown();
            // Important: reset the backend so a new one is created on next init
            s_backend.reset();
        }
    }
    
    std::vector<device_info> audio_system::enumerate_devices(bool playback_devices) {
        std::lock_guard<std::mutex> lock(s_system_mutex);
        
        if (!s_backend) {
            THROW_RUNTIME("Audio system not initialized. Call audio_system::init() first.");
        }
        
        // Use the existing audio_device API
        return audio_device::enumerate_devices(playback_devices);
    }
    
    device_info audio_system::get_default_device(bool playback_device) {
        std::lock_guard<std::mutex> lock(s_system_mutex);
        
        if (!s_backend) {
            THROW_RUNTIME("Audio system not initialized. Call audio_system::init() first.");
        }
        
        // Get default device info through enumeration
        auto devices = audio_device::enumerate_devices(playback_device);
        for (const auto& device : devices) {
            if (device.is_default) {
                return device;
            }
        }
        
        // If no default found, return the first device if available
        if (!devices.empty()) {
            return devices[0];
        }
        
        THROW_RUNTIME("No audio devices available.");
    }
    
    bool audio_system::switch_device(const device_info& device) {
        std::lock_guard<std::mutex> lock(s_system_mutex);
        
        if (!s_backend) {
            return false;
        }
        
        // TODO: Implement actual device switching logic in Phase 3/4
        // For now, just validate the device exists
        
        // Check if the device exists in the enumeration
        auto devices = audio_device::enumerate_devices(true);
        bool device_found = false;
        for (const auto& d : devices) {
            if (d.id == device.id) {
                device_found = true;
                break;
            }
        }
        
        if (!device_found) {
            return false;
        }
        
        // TODO: Implement actual switching
        return false; // Stub for now
    }
}
