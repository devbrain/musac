// This is copyrighted software. More information is at the end of this file.
#ifndef MUSAC_DEVICE_GUARD_HH
#define MUSAC_DEVICE_GUARD_HH

#include <musac/audio_device_interface.hh>
#include <memory>

namespace musac {
    
    // RAII guard for audio device handle cleanup
    // Ensures device is closed when the guard is destroyed
    class device_guard {
    public:
        device_guard() = default;
        
        device_guard(std::shared_ptr<audio_device_interface> manager, uint32_t handle)
            : m_manager(std::move(manager))
            , m_handle(handle) {
        }
        
        ~device_guard() {
            close();
        }
        
        // Move constructor
        device_guard(device_guard&& other) noexcept
            : m_manager(std::move(other.m_manager))
            , m_handle(other.m_handle) {
            other.m_handle = 0; // Mark as moved-from
        }
        
        // Move assignment
        device_guard& operator=(device_guard&& other) noexcept {
            if (this != &other) {
                close(); // Close current device first
                m_manager = std::move(other.m_manager);
                m_handle = other.m_handle;
                other.m_handle = 0;
            }
            return *this;
        }
        
        // Delete copy operations
        device_guard(const device_guard&) = delete;
        device_guard& operator=(const device_guard&) = delete;
        
        // Get the handle (for use with manager)
        uint32_t handle() const { return m_handle; }
        
        // Check if guard holds a valid device
        bool valid() const { return m_manager && m_handle != 0; }
        
        // Get the manager (for creating streams, etc.)
        std::shared_ptr<audio_device_interface> manager() const { return m_manager; }
        
        // Manually close the device (called automatically in destructor)
        void close() {
            if (m_manager && m_handle != 0) {
                m_manager->close_device(m_handle);
                m_handle = 0;
            }
        }
        
        // Release ownership without closing
        uint32_t release() {
            uint32_t h = m_handle;
            m_handle = 0;
            m_manager.reset();
            return h;
        }
        
    private:
        std::shared_ptr<audio_device_interface> m_manager;
        uint32_t m_handle = 0;
    };
    
} // namespace musac

#endif // MUSAC_DEVICE_GUARD_HH

/*
BSD Zero Clause License

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/