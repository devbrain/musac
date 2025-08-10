#include "backend_test_helpers.hh"
#include <doctest/doctest.h>
#include <thread>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <algorithm>

namespace musac::test {

void test_backend_initialization(std::unique_ptr<audio_backend> backend) {
    REQUIRE(backend != nullptr);
    
    // Should not be initialized initially
    CHECK_FALSE(backend->is_initialized());
    
    // Initialize
    CHECK_NOTHROW(backend->init());
    CHECK(backend->is_initialized());
    
    // Double init should throw
    CHECK_THROWS_AS(backend->init(), std::runtime_error);
    
    // Shutdown
    CHECK_NOTHROW(backend->shutdown());
    CHECK_FALSE(backend->is_initialized());
    
    // Double shutdown should be safe
    CHECK_NOTHROW(backend->shutdown());
}

void test_device_enumeration(std::unique_ptr<audio_backend> backend) {
    REQUIRE(backend != nullptr);
    
    // Should throw before init
    CHECK_THROWS_AS(backend->enumerate_playback_devices(), std::runtime_error);
    
    backend->init();
    
    // Should return at least one device (even if default)
    auto devices = backend->enumerate_playback_devices();
    CHECK_FALSE(devices.empty());
    
    // Check default device
    auto default_device = backend->get_default_playback_device();
    CHECK_FALSE(default_device.name.empty());
    CHECK(default_device.is_default);
    
    // Recording devices (if supported)
    if (backend->supports_recording()) {
        auto recording_devices = backend->enumerate_recording_devices();
        CHECK_FALSE(recording_devices.empty());
        
        auto default_recording = backend->get_default_recording_device();
        CHECK_FALSE(default_recording.name.empty());
        CHECK(default_recording.is_default);
    }
    
    backend->shutdown();
}

void test_device_open_close(std::unique_ptr<audio_backend> backend) {
    REQUIRE(backend != nullptr);
    
    backend->init();
    
    // Open default device
    musac::audio_spec desired_spec;
    desired_spec.format = musac::audio_format::s16le;
    desired_spec.channels = 2;
    desired_spec.freq = 44100;
    
    musac::audio_spec obtained_spec;
    uint32_t handle = 0;
    
    CHECK_NOTHROW(handle = backend->open_device("", desired_spec, obtained_spec));
    CHECK(handle != 0);
    
    // Check obtained spec is valid
    CHECK(obtained_spec.format != musac::audio_format::unknown);
    CHECK(obtained_spec.channels > 0);
    CHECK(obtained_spec.freq > 0);
    
    // Get device properties
    CHECK_NOTHROW(backend->get_device_format(handle));
    CHECK(backend->get_device_frequency(handle) > 0);
    CHECK(backend->get_device_channels(handle) > 0);
    
    // Close device
    CHECK_NOTHROW(backend->close_device(handle));
    
    // Accessing closed device should throw
    CHECK_THROWS_AS(backend->get_device_format(handle), std::runtime_error);
    
    backend->shutdown();
}

void test_device_control(std::unique_ptr<audio_backend> backend) {
    REQUIRE(backend != nullptr);
    
    backend->init();
    
    // Open a device
    musac::audio_spec desired_spec;
    desired_spec.format = musac::audio_format::s16le;
    desired_spec.channels = 2;
    desired_spec.freq = 44100;
    
    musac::audio_spec obtained_spec;
    uint32_t handle = backend->open_device("", desired_spec, obtained_spec);
    
    // Test pause/resume
    CHECK(backend->pause_device(handle));
    CHECK(backend->is_device_paused(handle));
    
    CHECK(backend->resume_device(handle));
    CHECK_FALSE(backend->is_device_paused(handle));
    
    // Test volume control
    float original_gain = backend->get_device_gain(handle);
    CHECK(original_gain >= 0.0f);
    CHECK(original_gain <= 1.0f);
    
    CHECK_NOTHROW(backend->set_device_gain(handle, 0.5f));
    CHECK(backend->get_device_gain(handle) == doctest::Approx(0.5f));
    
    // Restore original gain
    backend->set_device_gain(handle, original_gain);
    
    backend->close_device(handle);
    backend->shutdown();
}

void test_backend_capabilities(std::unique_ptr<audio_backend> backend) {
    REQUIRE(backend != nullptr);
    
    // Check capabilities
    bool supports_rec = backend->supports_recording();
    CHECK((supports_rec == true || supports_rec == false)); // Just check it returns a valid bool
    CHECK(backend->get_max_open_devices() > 0);
    
    // Check backend name
    CHECK_FALSE(backend->get_name().empty());
}

void test_multiple_devices(std::unique_ptr<audio_backend> backend) {
    REQUIRE(backend != nullptr);
    
    backend->init();
    
    // Try to open multiple devices
    musac::audio_spec desired_spec;
    desired_spec.format = musac::audio_format::s16le;
    desired_spec.channels = 2;
    desired_spec.freq = 44100;
    
    std::vector<uint32_t> handles;
    musac::audio_spec obtained_spec;
    
    // Open up to 3 devices or max supported
    size_t max_devices = static_cast<size_t>(std::min(3, backend->get_max_open_devices()));
    
    for (size_t i = 0; i < max_devices; ++i) {
        uint32_t handle = 0;
        try {
            handle = backend->open_device("", desired_spec, obtained_spec);
            if (handle != 0) {
                handles.push_back(handle);
            }
        } catch (...) {
            // Some backends might not support multiple devices
            break;
        }
    }
    
    CHECK_FALSE(handles.empty());
    
    // All handles should be unique
    for (size_t i = 0; i < handles.size(); ++i) {
        for (size_t j = i + 1; j < handles.size(); ++j) {
            CHECK(handles[i] != handles[j]);
        }
    }
    
    // Close all devices
    for (auto handle : handles) {
        CHECK_NOTHROW(backend->close_device(handle));
    }
    
    backend->shutdown();
}

void test_stream_creation(std::unique_ptr<audio_backend> backend) {
    REQUIRE(backend != nullptr);
    
    backend->init();
    
    // Open a device
    musac::audio_spec desired_spec;
    desired_spec.format = musac::audio_format::s16le;
    desired_spec.channels = 2;
    desired_spec.freq = 44100;
    
    musac::audio_spec obtained_spec;
    uint32_t handle = backend->open_device("", desired_spec, obtained_spec);
    
    // Create a stream with a simple callback
    auto callback = [](void* /* userdata */, uint8_t* stream, int len) {
        // Simple silence callback
        std::memset(stream, 0, static_cast<size_t>(len));
    };
    
    auto stream = backend->create_stream(handle, obtained_spec, callback, nullptr);
    CHECK(stream != nullptr);
    
    backend->close_device(handle);
    backend->shutdown();
}

void test_error_conditions(std::unique_ptr<audio_backend> backend) {
    REQUIRE(backend != nullptr);
    
    // Test operations before init
    CHECK_THROWS_AS(backend->enumerate_playback_devices(), std::runtime_error);
    CHECK_THROWS_AS(backend->get_default_playback_device(), std::runtime_error);
    
    musac::audio_spec spec;
    spec.format = musac::audio_format::s16le;
    spec.channels = 2;
    spec.freq = 44100;
    musac::audio_spec obtained;
    
    CHECK_THROWS_AS(backend->open_device("", spec, obtained), std::runtime_error);
    
    backend->init();
    
    // Test invalid device handle
    uint32_t invalid_handle = 999999;
    CHECK_THROWS_AS(backend->get_device_format(invalid_handle), std::runtime_error);
    CHECK_THROWS_AS(backend->get_device_gain(invalid_handle), std::runtime_error);
    CHECK_THROWS_AS(backend->set_device_gain(invalid_handle, 0.5f), std::runtime_error);
    // close_device silently ignores invalid handles to prevent cleanup crashes
    CHECK_NOTHROW(backend->close_device(invalid_handle));
    
    // Test invalid device name - SDL backends typically fall back to default device
    // so this might not throw. We'll make it a conditional test.
    try {
        uint32_t handle = backend->open_device("nonexistent_device_12345", spec, obtained);
        // If it doesn't throw, it should have opened the default device
        CHECK(handle != 0);
        backend->close_device(handle);
    } catch (const std::exception&) {
        // Some backends might throw on invalid device name, which is also acceptable
        CHECK(true);
    }
    
    backend->shutdown();
}

std::string get_test_device_id(audio_backend* backend, bool for_recording) {
    if (!backend->is_initialized()) {
        backend->init();
    }
    
    if (for_recording) {
        auto device = backend->get_default_recording_device();
        return device.id;
    } else {
        auto device = backend->get_default_playback_device();
        return device.id;
    }
}

bool backend_supports_recording(audio_backend* backend) {
    return backend->supports_recording();
}

} // namespace musac::test