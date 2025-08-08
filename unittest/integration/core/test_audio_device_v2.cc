#include <doctest/doctest.h>
#include <musac/audio_device.hh>
#include <musac/audio_system.hh>
#include <musac/backends/sdl3/sdl3_backend.hh>
#include <musac/sdk/audio_backend_v2.hh>
#include <musac/sdk/audio_format.hh>
#include <vector>
#include <stdexcept>

using namespace musac;

TEST_SUITE("AudioDevice::BackendV2::Integration") {
    
    TEST_CASE("enumerate devices with v2 backend") {
        // Create a backend explicitly
        std::shared_ptr<audio_backend_v2> backend(create_sdl3_backend_v2());
        REQUIRE(backend.get() != nullptr);
        
        // Initialize the backend
        backend->init();
        REQUIRE(backend->is_initialized());
        
        // Enumerate devices using new API
        auto devices = audio_device::enumerate_devices(backend, true);
        REQUIRE(devices.size() > 0);
        
        // Check device properties
        bool found_default = false;
        for (const auto& dev : devices) {
            if (dev.is_default) {
                found_default = true;
                CHECK(dev.name.size() > 0);
                CHECK(dev.id.size() > 0);
                CHECK(dev.channels > 0);
                CHECK(dev.sample_rate > 0);
            }
        }
        REQUIRE(found_default);
        
        backend->shutdown();
    }
    
    TEST_CASE("open default device with v2 backend") {
        std::shared_ptr<audio_backend_v2> backend(create_sdl3_backend_v2());
        backend->init();
        
        // Open default device with new API
        audio_spec spec;
        spec.format = audio_format::f32le;
        spec.channels = 2;
        spec.freq = 44100;
        
        auto device = audio_device::open_default_device(backend, &spec);
        
        // Check device properties
        CHECK(device.get_channels() == 2);
        CHECK(device.get_freq() == 44100);
        CHECK(device.get_format() == audio_format::f32le);
        CHECK(device.get_device_name().size() > 0);
        CHECK(device.get_device_id().size() > 0);
        
        backend->shutdown();
    }
    
    TEST_CASE("open specific device with v2 backend") {
        std::shared_ptr<audio_backend_v2> backend(create_sdl3_backend_v2());
        backend->init();
        
        // Get list of devices
        auto devices = audio_device::enumerate_devices(backend, true);
        REQUIRE(devices.size() > 0);
        
        // Open first device by ID
        audio_spec spec;
        spec.format = audio_format::s16le;
        spec.channels = 2;
        spec.freq = 48000;
        
        auto device = audio_device::open_device(backend, devices[0].id, &spec);
        
        // Check device opened
        CHECK(device.get_device_id() == devices[0].id);
        CHECK(device.get_channels() == 2);
        CHECK(device.get_freq() == 48000);
        CHECK(device.get_format() == audio_format::s16le);
        
        backend->shutdown();
    }
    
    TEST_CASE("device control operations with v2 backend") {
        std::shared_ptr<audio_backend_v2> backend(create_sdl3_backend_v2());
        backend->init();
        
        auto device = audio_device::open_default_device(backend);
        
        // Test pause/resume
        CHECK_FALSE(device.is_paused());
        CHECK(device.pause());
        CHECK(device.is_paused());
        CHECK(device.resume());
        CHECK_FALSE(device.is_paused());
        
        // Test gain control
        device.set_gain(0.5f);
        CHECK(device.get_gain() == doctest::Approx(0.5f));
        device.set_gain(1.0f);
        CHECK(device.get_gain() == doctest::Approx(1.0f));
        
        backend->shutdown();
    }
    
    TEST_CASE("multiple devices from same backend") {
        std::shared_ptr<audio_backend_v2> backend(create_sdl3_backend_v2());
        backend->init();
        
        auto devices = audio_device::enumerate_devices(backend, true);
        
        // SDL3 dummy driver may only provide one device, so skip test if not enough devices
        if (devices.size() < 2) {
            MESSAGE("Skipping test - not enough devices available (only ", devices.size(), " found)");
            return;
        }
        
        // Open two different devices
        auto device1 = audio_device::open_device(backend, devices[0].id);
        auto device2 = audio_device::open_device(backend, devices[1].id);
        
        // Check they have different IDs
        CHECK(device1.get_device_id() != device2.get_device_id());
        
        // Control them independently
        device1.set_gain(0.3f);
        device2.set_gain(0.7f);
        CHECK(device1.get_gain() == doctest::Approx(0.3f));
        CHECK(device2.get_gain() == doctest::Approx(0.7f));
        
        backend->shutdown();
    }
    
    TEST_CASE("v2 API with global backend") {
        // Initialize audio system with null backend
        std::shared_ptr<audio_backend_v2> backend(create_sdl3_backend_v2());
        audio_system::init(backend);
        
        // Get the backend from audio_system
        auto global_backend = audio_system::get_backend();
        REQUIRE(global_backend.get() != nullptr);
        
        // Use the v2 API with the global backend
        auto devices = audio_device::enumerate_devices(global_backend, true);
        REQUIRE(devices.size() > 0);
        
        auto device = audio_device::open_default_device(global_backend);
        CHECK(device.get_device_name().size() > 0);
        
        audio_system::done();
    }
    
    TEST_CASE("error handling with null backend") {
        std::shared_ptr<audio_backend_v2> backend(create_sdl3_backend_v2());
        
        // Try to enumerate before init
        CHECK_THROWS_AS(
            audio_device::enumerate_devices(backend),
            std::runtime_error
        );
        
        // Null backend should not throw
        CHECK_THROWS_AS(
            audio_device::enumerate_devices(nullptr),
            std::runtime_error
        );
        
        backend->init();
        
        // Try to open non-existent device
        CHECK_THROWS_AS(
            audio_device::open_device(backend, "non_existent_device_id"),
            std::runtime_error
        );
        
        backend->shutdown();
    }
}