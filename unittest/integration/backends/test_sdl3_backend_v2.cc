#include <doctest/doctest.h>
#include <musac/backends/sdl3/sdl3_backend.hh>
#include <musac/sdk/audio_backend_v2.hh>
#include <iostream>
#include <thread>
#include <chrono>

TEST_SUITE("Backend::SDL3::Integration") {
    TEST_CASE("SDL3 backend v2 creation") {
        auto backend = musac::create_sdl3_backend_v2();
        CHECK(backend != nullptr);
        CHECK_FALSE(backend->is_initialized());
        CHECK(backend->get_name() == "SDL3");
    }
    
    TEST_CASE("SDL3 backend v2 initialization") {
        auto backend = musac::create_sdl3_backend_v2();
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
    
    TEST_CASE("SDL3 backend v2 device enumeration") {
        auto backend = musac::create_sdl3_backend_v2();
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
        
        // Recording devices
        auto recording_devices = backend->enumerate_recording_devices();
        CHECK_FALSE(recording_devices.empty());
        
        backend->shutdown();
    }
    
    TEST_CASE("SDL3 backend v2 device open/close") {
        auto backend = musac::create_sdl3_backend_v2();
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
    
    TEST_CASE("SDL3 backend v2 device control") {
        auto backend = musac::create_sdl3_backend_v2();
        REQUIRE(backend != nullptr);
        
        backend->init();
        
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
        
        // Test gain control
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
    
    TEST_CASE("SDL3 backend v2 capabilities") {
        auto backend = musac::create_sdl3_backend_v2();
        REQUIRE(backend != nullptr);
        
        // SDL3 backend capabilities
        CHECK(backend->supports_recording());
        CHECK(backend->get_max_open_devices() > 0);
    }
    
    TEST_CASE("SDL3 backend v2 multiple devices") {
        auto backend = musac::create_sdl3_backend_v2();
        REQUIRE(backend != nullptr);
        
        backend->init();
        
        musac::audio_spec spec;
        spec.format = musac::audio_format::s16le;
        spec.channels = 2;
        spec.freq = 44100;
        
        // Open multiple devices
        std::vector<uint32_t> handles;
        musac::audio_spec obtained;
        
        // Try to open a few devices
        for (int i = 0; i < 3; ++i) {
            try {
                uint32_t handle = backend->open_device("", spec, obtained);
                handles.push_back(handle);
            } catch (...) {
                // Some systems may not support multiple devices
                break;
            }
        }
        
        CHECK_FALSE(handles.empty());
        
        // Each handle should be unique
        for (size_t i = 0; i < handles.size(); ++i) {
            for (size_t j = i + 1; j < handles.size(); ++j) {
                CHECK(handles[i] != handles[j]);
            }
        }
        
        // Close all devices
        for (uint32_t handle : handles) {
            CHECK_NOTHROW(backend->close_device(handle));
        }
        
        backend->shutdown();
    }
    
    TEST_CASE("SDL3 backend v2 stream creation") {
        auto backend = musac::create_sdl3_backend_v2();
        REQUIRE(backend != nullptr);
        
        backend->init();
        
        musac::audio_spec spec;
        spec.format = musac::audio_format::s16le;
        spec.channels = 2;
        spec.freq = 44100;
        
        musac::audio_spec obtained;
        uint32_t handle = backend->open_device("", spec, obtained);
        
        // Stream creation is now fully implemented
        // Check that we get a valid stream
        auto stream = backend->create_stream(handle, spec);
        // Should return a valid stream
        CHECK(stream != nullptr);
        
        backend->close_device(handle);
        backend->shutdown();
    }
}