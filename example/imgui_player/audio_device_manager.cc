#include "audio_device_manager.hh"
#include <thread>
#include <chrono>
#include <sstream>

namespace musac::player {

void AudioDeviceManager::init(const std::shared_ptr<audio_backend>& backend) {
    m_backend = backend;
    refresh_device_list();
    switch_to_default_device();
}

void AudioDeviceManager::refresh_device_list() {
    if (!m_backend) return;
    
    m_devices = m_backend->enumerate_devices(true);
    
    // If current device is no longer available, reset
    if (m_current_device_index >= static_cast<int>(m_devices.size())) {
        m_current_device_index = -1;
    }
}

bool AudioDeviceManager::switch_device(int device_index) {
    if (!m_backend || device_index < 0 || device_index >= static_cast<int>(m_devices.size())) {
        return false;
    }
    
    if (device_index == m_current_device_index && m_device_open) {
        return true; // Already on this device
    }
    
    m_switching_device = true;
    
    // Close current device if open
    if (m_device) {
        m_device.reset();  // This will destroy the device
        m_device_open = false;
        // Small delay to ensure clean shutdown
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Open new device
    try {
        const auto& device_info = m_devices[device_index];
        auto new_device = std::make_unique<audio_device>(
            audio_device::open_device(m_backend, device_info.id));
        new_device->resume();
        m_device = std::move(new_device);
        m_device_open = true;
        m_current_device_index = device_index;
        m_switching_device = false;
        return true;
    } catch (const std::exception& e) {
        m_switching_device = false;
        m_current_device_index = -1;
        m_device_open = false;
        return false;
    }
}

bool AudioDeviceManager::switch_to_default_device() {
    refresh_device_list();
    
    // Find default device
    for (size_t i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i].is_default) {
            return switch_device(static_cast<int>(i));
        }
    }
    
    // Fallback to first device if no default found
    if (!m_devices.empty()) {
        return switch_device(0);
    }
    
    return false;
}

std::string AudioDeviceManager::get_device_display_name(int index) const {
    if (index < 0 || index >= static_cast<int>(m_devices.size())) {
        return "Invalid Device";
    }
    
    const auto& device = m_devices[index];
    std::stringstream ss;
    ss << device.name;
    
    if (device.is_default) {
        ss << " (Default)";
    }
    
    if (index == m_current_device_index) {
        ss << " [Current]";
    }
    
    return ss.str();
}

std::string AudioDeviceManager::get_current_device_name() const {
    if (m_current_device_index < 0 || m_current_device_index >= static_cast<int>(m_devices.size())) {
        return "No Device";
    }
    return m_devices[m_current_device_index].name;
}

std::string AudioDeviceManager::get_backend_name() const {
    if (!m_backend) {
        return "No Backend";
    }
    return m_backend->get_name();
}

} // namespace musac::player