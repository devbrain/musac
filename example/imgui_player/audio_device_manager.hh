#pragma once

#include <memory>
#include <vector>
#include <string>
#include <musac/audio_device.hh>
#include <musac/sdk/audio_backend.hh>

namespace musac::player {

/**
 * AudioDeviceManager - Manages audio device enumeration and switching
 * 
 * Responsibilities:
 * - Enumerate available audio devices
 * - Handle device switching with proper cleanup
 * - Provide current device information
 * - Abstract backend-specific details
 */
class AudioDeviceManager {
public:
    AudioDeviceManager() = default;
    ~AudioDeviceManager() = default;

    // Initialize with backend
    void init(const std::shared_ptr<audio_backend>& backend);
    
    // Device operations
    void refresh_device_list();
    bool switch_device(int device_index);
    bool switch_to_default_device();
    
    // Getters
    audio_device* get_device() { return m_device.get(); }
    const audio_device* get_device() const { return m_device.get(); }
    audio_device& get_current_device() { return *m_device; }
    const audio_device& get_current_device() const { return *m_device; }
    const std::vector<device_info>& get_device_list() const { return m_devices; }
    int get_current_device_index() const { return m_current_device_index; }
    
    // Device info helpers
    std::string get_device_display_name(int index) const;
    std::string get_current_device_name() const;
    bool is_device_open() const { return m_device_open; }
    bool is_switching() const { return m_switching_device; }
    
    // Backend info
    std::string get_backend_name() const;

private:
    std::shared_ptr<audio_backend> m_backend;
    std::unique_ptr<audio_device> m_device;
    std::vector<device_info> m_devices;
    int m_current_device_index = -1;
    bool m_switching_device = false;
    bool m_device_open = false;
};

} // namespace musac::player