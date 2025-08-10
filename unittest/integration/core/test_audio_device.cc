#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <thread>
#include <chrono>
#include <stdexcept>
#include "../../mock_components.hh"
#include "../../test_fixtures.hh"

namespace musac::test {

TEST_SUITE("AudioDevice::Integration") {
    // Use v2 test fixture
    using audio_test_fixture = test::audio_test_fixture_v2;
    
    TEST_CASE_FIXTURE(audio_test_fixture, "device enumeration") {
        auto devices = audio_device::enumerate_devices(backend, true);
        
        // Should have at least one device (even if it's just a null device)
        CHECK(devices.size() > 0);
        
        // Check default device exists
        bool has_default = false;
        for (const auto& dev : devices) {
            if (dev.is_default) {
                has_default = true;
                break;
            }
        }
        CHECK(has_default);
        
        // Verify device info
        for (const auto& dev : devices) {
            CHECK(!dev.name.empty());
            CHECK(!dev.id.empty());
            CHECK(dev.channels > 0);
            CHECK(dev.channels <= 8); // Reasonable channel count
            CHECK(dev.sample_rate > 0);
            CHECK(dev.sample_rate <= 192000); // Reasonable sample rate
        }
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "open default device") {
        auto device = audio_device::open_default_device(backend);
        
        // Device should be valid
        CHECK(device.get_channels() > 0);
        CHECK(device.get_freq() > 0);
        CHECK(device.get_format() != audio_format::unknown);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "open device with custom spec") {
        audio_spec desired;
        desired.format = audio_format::f32le;
        desired.channels = 2;
        desired.freq = 48000;
        
        auto device = audio_device::open_default_device(backend, &desired);
        
        // Device should respect requested spec or provide compatible alternative
        CHECK(device.get_channels() > 0);
        CHECK(device.get_freq() > 0);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "device pause/resume") {
        auto device = audio_device::open_default_device(backend);
        
        CHECK_FALSE(device.is_paused());
        
        CHECK(device.pause());
        CHECK(device.is_paused());
        
        CHECK(device.resume());
        CHECK_FALSE(device.is_paused());
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "device gain control") {
        auto device = audio_device::open_default_device(backend);
        
        float initial_gain = device.get_gain();
        CHECK(initial_gain >= 0.0f);
        CHECK(initial_gain <= 1.0f);
        
        device.set_gain(0.5f);
        CHECK(device.get_gain() == doctest::Approx(0.5f).epsilon(0.01));
        
        device.set_gain(1.0f);
        CHECK(device.get_gain() == doctest::Approx(1.0f).epsilon(0.01));
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "multiple device instances") {
        // Multiple devices can now be created (for device switching support)
        auto device1 = audio_device::open_default_device(backend);
        CHECK(device1.get_channels() > 0);
        
        // Creating a second device should now succeed
        auto device2 = audio_device::open_default_device(backend);
        CHECK(device2.get_channels() > 0);
        
        // Operations on both devices should work
        device1.pause();
        CHECK(device1.is_paused());
        device1.resume();
        CHECK_FALSE(device1.is_paused());
        
        // device2 operations also work
        device2.pause();
        CHECK(device2.is_paused());
        device2.resume();
        CHECK_FALSE(device2.is_paused());
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "device move semantics") {
        auto device1 = audio_device::open_default_device(backend);
        auto channels = device1.get_channels();
        
        // Move construction
        audio_device device2(std::move(device1));
        CHECK(device2.get_channels() == channels);
        
        // Original should be moved-from
        // Note: We can't test device1 anymore as it's in moved-from state
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "device destruction order") {
        // Test device switching - can only have one device at a time
        {
            auto device1 = audio_device::open_default_device(backend);
            
            // Create stream from device
            auto source1 = mock_audio_source::create();
            auto stream1 = device1.create_stream(std::move(*source1));
            
            // Device and stream will be destroyed when going out of scope
        }
        
        // Now we can create a new device
        {
            auto device2 = audio_device::open_default_device(backend);
            
            // Create stream from new device
            auto source2 = mock_audio_source::create();
            auto stream2 = device2.create_stream(std::move(*source2));
            
            // This should work fine
        }
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "open non-existent device") {
        // Try to open a device with invalid ID
        CHECK_THROWS(audio_device::open_device(backend, "non_existent_device_id_12345"));
    }
}

TEST_SUITE("AudioDevice::BackendV2::Integration") {

    TEST_CASE("enumerate devices with v2 backend") {
        // Create a backend explicitly
        std::shared_ptr<audio_backend> backend(create_sdl3_backend());
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
        std::shared_ptr<audio_backend> backend(create_sdl3_backend());
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
        std::shared_ptr<audio_backend> backend(create_sdl3_backend());
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
        std::shared_ptr<audio_backend> backend(create_sdl3_backend());
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
        std::shared_ptr<audio_backend> backend(create_sdl3_backend());
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
        std::shared_ptr<audio_backend> backend(create_sdl3_backend());
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
        std::shared_ptr<audio_backend> backend(create_sdl3_backend());

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

} // namespace musac::test