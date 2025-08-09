#include <doctest/doctest.h>
#include <musac/sdk/audio_backend.hh>
#include <musac/audio_stream_interface.hh>
#include <memory>
#include <vector>
#include <stdexcept>
#include <map>

namespace musac::test {

// Mock implementation for testing the interface contract
class mock_backend_v2 : public audio_backend {
private:
    bool m_initialized = false;
    std::string m_name;
    std::vector<device_info_v2> m_devices;
    std::map<uint32_t, audio_spec> m_open_devices;
    uint32_t m_next_handle = 1;
    
public:
    explicit mock_backend_v2(const std::string& name = "MockBackend") 
        : m_name(name) {
        // Setup some mock devices
        m_devices.push_back({"Default Device", "default", true, 2, 44100});
        m_devices.push_back({"USB Audio", "usb_audio", false, 2, 48000});
        m_devices.push_back({"HDMI Output", "hdmi", false, 6, 48000});
    }
    
    // Initialization
    void init() override {
        if (m_initialized) {
            throw std::runtime_error("Backend already initialized");
        }
        m_initialized = true;
    }
    
    void shutdown() override {
        if (!m_initialized) {
            return; // Allow shutdown on non-initialized backend
        }
        // Close all open devices
        m_open_devices.clear();
        m_initialized = false;
    }
    
    std::string get_name() const override {
        return m_name;
    }
    
    bool is_initialized() const override {
        return m_initialized;
    }
    
    // Device enumeration
    std::vector<device_info_v2> enumerate_devices(bool /*playback*/) override {
        if (!m_initialized) {
            throw std::runtime_error("Backend not initialized");
        }
        // For mock, return same devices for playback and recording
        return m_devices;
    }
    
    device_info_v2 get_default_device(bool /*playback*/) override {
        if (!m_initialized) {
            throw std::runtime_error("Backend not initialized");
        }
        return m_devices[0]; // First device is default
    }
    
    // Device management
    uint32_t open_device(const std::string& device_id, 
                        const audio_spec& spec, 
                        audio_spec& obtained_spec) override {
        if (!m_initialized) {
            throw std::runtime_error("Backend not initialized");
        }
        
        // Find the device
        bool found = device_id.empty() || device_id == "default";
        if (!found) {
            for (const auto& dev : m_devices) {
                if (dev.id == device_id) {
                    found = true;
                    break;
                }
            }
        }
        
        if (!found) {
            throw std::runtime_error("Device not found: " + device_id);
        }
        
        // Mock: always return the requested spec
        obtained_spec = spec;
        
        uint32_t handle = m_next_handle++;
        m_open_devices[handle] = obtained_spec;
        return handle;
    }
    
    void close_device(uint32_t device_handle) override {
        m_open_devices.erase(device_handle);
    }
    
    // Device properties
    audio_format get_device_format(uint32_t device_handle) override {
        auto it = m_open_devices.find(device_handle);
        if (it == m_open_devices.end()) {
            throw std::runtime_error("Invalid device handle");
        }
        return it->second.format;
    }
    
    uint32_t get_device_frequency(uint32_t device_handle) override {
        auto it = m_open_devices.find(device_handle);
        if (it == m_open_devices.end()) {
            throw std::runtime_error("Invalid device handle");
        }
        return it->second.freq;
    }
    
    uint8_t get_device_channels(uint32_t device_handle) override {
        auto it = m_open_devices.find(device_handle);
        if (it == m_open_devices.end()) {
            throw std::runtime_error("Invalid device handle");
        }
        return it->second.channels;
    }
    
    float get_device_gain(uint32_t device_handle) override {
        if (m_open_devices.find(device_handle) == m_open_devices.end()) {
            throw std::runtime_error("Invalid device handle");
        }
        return 1.0f; // Mock: always full volume
    }
    
    void set_device_gain(uint32_t device_handle, float /*gain*/) override {
        if (m_open_devices.find(device_handle) == m_open_devices.end()) {
            throw std::runtime_error("Invalid device handle");
        }
        // Mock: accept but don't store
    }
    
    // Device control
    bool pause_device(uint32_t device_handle) override {
        return m_open_devices.find(device_handle) != m_open_devices.end();
    }
    
    bool resume_device(uint32_t device_handle) override {
        return m_open_devices.find(device_handle) != m_open_devices.end();
    }
    
    bool is_device_paused(uint32_t device_handle) override {
        if (m_open_devices.find(device_handle) == m_open_devices.end()) {
            throw std::runtime_error("Invalid device handle");
        }
        return false; // Mock: never paused
    }
    
    // Stream creation
    std::unique_ptr<audio_stream_interface> create_stream(
        uint32_t device_handle,
        const audio_spec& /*spec*/,
        void (*/*callback*/)(void* userdata, uint8_t* stream, int len),
        void* /*userdata*/) override {
        
        if (m_open_devices.find(device_handle) == m_open_devices.end()) {
            throw std::runtime_error("Invalid device handle");
        }
        
        // Return nullptr for mock - actual implementation would create a stream
        return nullptr;
    }
    
    // Capabilities
    bool supports_recording() const override {
        return true;
    }
    
    int get_max_open_devices() const override {
        return 8; // Mock: arbitrary limit
    }
};

} // namespace musac::test

TEST_SUITE("Backend::Interface::Integration") {
    using namespace musac;
    using namespace musac::test;
    
    TEST_CASE("Backend initialization lifecycle") {
        mock_backend_v2 backend("TestBackend");
        
        SUBCASE("Initial state") {
            CHECK(backend.get_name() == "TestBackend");
            CHECK(backend.is_initialized() == false);
        }
        
        SUBCASE("Initialization") {
            backend.init();
            CHECK(backend.is_initialized() == true);
            
            // Double init should throw
            CHECK_THROWS_AS(backend.init(), std::runtime_error);
        }
        
        SUBCASE("Shutdown") {
            backend.init();
            backend.shutdown();
            CHECK(backend.is_initialized() == false);
            
            // Multiple shutdowns should be safe
            CHECK_NOTHROW(backend.shutdown());
        }
    }
    
    TEST_CASE("Device enumeration") {
        mock_backend_v2 backend;
        
        SUBCASE("Enumeration requires initialization") {
            CHECK_THROWS_AS(backend.enumerate_devices(true), std::runtime_error);
            CHECK_THROWS_AS(backend.get_default_device(true), std::runtime_error);
        }
        
        SUBCASE("Enumerate devices") {
            backend.init();
            
            auto devices = backend.enumerate_devices(true);
            CHECK(devices.size() > 0);
            
            // Check default device
            auto default_dev = backend.get_default_device(true);
            CHECK(default_dev.is_default == true);
            CHECK(!default_dev.id.empty());
            CHECK(!default_dev.name.empty());
        }
        
        SUBCASE("Convenience wrappers") {
            backend.init();
            
            // Test convenience methods
            auto playback_devices = backend.enumerate_playback_devices();
            CHECK(playback_devices.size() > 0);
            
            auto recording_devices = backend.enumerate_recording_devices();
            CHECK(recording_devices.size() > 0); // Mock returns same devices
            
            auto default_playback = backend.get_default_playback_device();
            CHECK(default_playback.is_default == true);
            
            auto default_recording = backend.get_default_recording_device();
            CHECK(default_recording.is_default == true);
        }
    }
    
    TEST_CASE("Device management") {
        mock_backend_v2 backend;
        backend.init();
        
        audio_spec requested_spec;
        requested_spec.format = audio_format::s16le;
        requested_spec.channels = 2;
        requested_spec.freq = 44100;
        
        SUBCASE("Open default device") {
            audio_spec obtained_spec;
            auto handle = backend.open_device("", requested_spec, obtained_spec);
            
            CHECK(handle != 0);
            CHECK(obtained_spec.format == requested_spec.format);
            CHECK(obtained_spec.channels == requested_spec.channels);
            CHECK(obtained_spec.freq == requested_spec.freq);
            
            backend.close_device(handle);
        }
        
        SUBCASE("Open specific device") {
            audio_spec obtained_spec;
            auto handle = backend.open_device("usb_audio", requested_spec, obtained_spec);
            
            CHECK(handle != 0);
            
            // Check device properties
            CHECK(backend.get_device_format(handle) == requested_spec.format);
            CHECK(backend.get_device_frequency(handle) == requested_spec.freq);
            CHECK(backend.get_device_channels(handle) == requested_spec.channels);
            
            backend.close_device(handle);
        }
        
        SUBCASE("Invalid device") {
            audio_spec obtained_spec;
            CHECK_THROWS_AS(
                backend.open_device("nonexistent", requested_spec, obtained_spec),
                std::runtime_error
            );
        }
    }
    
    TEST_CASE("Device control") {
        mock_backend_v2 backend;
        backend.init();
        
        audio_spec spec;
        spec.format = audio_format::s16le;
        spec.channels = 2;
        spec.freq = 44100;
        
        audio_spec obtained_spec;
        auto handle = backend.open_device("", spec, obtained_spec);
        
        SUBCASE("Pause and resume") {
            CHECK(backend.pause_device(handle) == true);
            CHECK(backend.resume_device(handle) == true);
            CHECK(backend.is_device_paused(handle) == false);
        }
        
        SUBCASE("Volume control") {
            CHECK(backend.get_device_gain(handle) == 1.0f);
            CHECK_NOTHROW(backend.set_device_gain(handle, 0.5f));
        }
        
        SUBCASE("Invalid handle") {
            uint32_t invalid_handle = 9999;
            CHECK_THROWS_AS(backend.get_device_gain(invalid_handle), std::runtime_error);
            CHECK_THROWS_AS(backend.set_device_gain(invalid_handle, 0.5f), std::runtime_error);
        }
        
        backend.close_device(handle);
    }
    
    TEST_CASE("Backend capabilities") {
        mock_backend_v2 backend;
        
        CHECK(backend.supports_recording() == true);
        CHECK(backend.get_max_open_devices() > 0);
    }
    
    TEST_CASE("Stream creation") {
        mock_backend_v2 backend;
        backend.init();
        
        audio_spec spec;
        spec.format = audio_format::s16le;
        spec.channels = 2;
        spec.freq = 44100;
        
        audio_spec obtained_spec;
        auto handle = backend.open_device("", spec, obtained_spec);
        
        SUBCASE("Create stream without callback") {
            // Mock returns nullptr, but real implementation would return a stream
            auto stream = backend.create_stream(handle, spec, nullptr, nullptr);
            CHECK(stream == nullptr); // Expected for mock
        }
        
        SUBCASE("Create stream using convenience wrapper") {
            // Test the non-virtual convenience method
            // Note: This tests that the convenience wrapper correctly delegates to the virtual method
            // Need to cast to base class to access the convenience method
            audio_backend& base = backend;
            auto stream = base.create_stream(handle, spec);
            CHECK(stream == nullptr); // Expected for mock
        }
        
        SUBCASE("Invalid device handle") {
            uint32_t invalid_handle = 9999;
            CHECK_THROWS_AS(
                backend.create_stream(invalid_handle, spec, nullptr, nullptr),
                std::runtime_error
            );
        }
        
        backend.close_device(handle);
    }
}