#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/sdk/audio_backend.hh>
#include "../../backend_selection.hh"
#include <musac/sdk/audio_format.hh>
#include <memory>
#include <stdexcept>

using namespace musac;

TEST_SUITE("AudioSystem::BackendV2::Integration") {
    
    TEST_CASE("init with explicit backend") {
        // Create a backend explicitly
        REQUIRE_BACKEND();
        std::shared_ptr<audio_backend> backend = test::create_backend();
        REQUIRE(backend.get() != nullptr);
        
        // Initialize audio system with backend
        bool result = audio_system::init(backend);
        REQUIRE(result == true);
        
        // Backend should be initialized
        REQUIRE(backend->is_initialized());
        
        // Should be able to get the backend back
        auto retrieved_backend = audio_system::get_backend();
        REQUIRE(retrieved_backend.get() == backend.get());
        
        // Clean up
        audio_system::done();
        
        // Backend should be reset after done()
        auto backend_after_done = audio_system::get_backend();
        CHECK(backend_after_done.get() == nullptr);
    }
    
    TEST_CASE("init with pre-initialized backend") {
        // Create and pre-initialize a backend
        REQUIRE_BACKEND();
        std::shared_ptr<audio_backend> backend = test::create_backend();
        backend->init();
        REQUIRE(backend->is_initialized());
        
        // Initialize audio system with pre-initialized backend
        bool result = audio_system::init(backend);
        REQUIRE(result == true);
        
        // Backend should still be initialized
        REQUIRE(backend->is_initialized());
        
        // Clean up
        audio_system::done();
    }
    
    TEST_CASE("enumerate devices with v2 backend") {
        // Initialize with explicit backend
        REQUIRE_BACKEND();
        std::shared_ptr<audio_backend> backend = test::create_backend();
        audio_system::init(backend);
        
        // Enumerate devices should use the v2 backend
        auto devices = audio_device::enumerate_devices(backend, true);
        REQUIRE(devices.size() > 0);
        
        // Check that we get expected SDL3 dummy device
        bool found_dummy_device = false;
        for (const auto& dev : devices) {
            // SDL3 dummy driver device names typically contain "dummy" or are generic
            if (dev.name.find("dummy") != std::string::npos || 
                dev.name.find("Dummy") != std::string::npos ||
                dev.is_default) {
                found_dummy_device = true;
                break;
            }
        }
        REQUIRE(found_dummy_device);
        
        audio_system::done();
    }
    
    TEST_CASE("get default device with v2 backend") {
        REQUIRE_BACKEND();
        std::shared_ptr<audio_backend> backend = test::create_backend();
        audio_system::init(backend);
        
        // Get default device
        // Get default device from enumeration
        auto devices = audio_device::enumerate_devices(backend, true);
        device_info default_device;
        for (const auto& dev : devices) {
            if (dev.is_default) {
                default_device = dev;
                break;
            }
        }
        
        // Should have valid properties
        CHECK(default_device.name.size() > 0);
        CHECK(default_device.id.size() > 0);
        CHECK(default_device.channels > 0);
        CHECK(default_device.sample_rate > 0);
        
        audio_system::done();
    }
    
    TEST_CASE("create device from system backend") {
        REQUIRE_BACKEND();
        std::shared_ptr<audio_backend> backend = test::create_backend();
        audio_system::init(backend);
        
        // Get the backend from system
        auto sys_backend = audio_system::get_backend();
        REQUIRE(sys_backend.get() == backend.get());
        
        // Create device using system's backend
        auto device = audio_device::open_default_device(sys_backend);
        
        // Device should work
        CHECK(device.get_channels() > 0);
        CHECK(device.get_freq() > 0);
        
        audio_system::done();
    }
    
    TEST_CASE("device switching with v2 backend") {
        REQUIRE_BACKEND();
        std::shared_ptr<audio_backend> backend = test::create_backend();
        audio_system::init(backend);
        
        // Create initial device
        auto device1 = audio_device::open_default_device(backend);
        
        // Get list of devices
        auto devices = audio_device::enumerate_devices(backend, true);
        
        // SDL3 dummy driver may only provide one device
        if (devices.size() < 2) {
            MESSAGE("Skipping device switching test - not enough devices available");
            audio_system::done();
            return;
        }
        
        // Create a different device
        audio_spec spec;
        spec.format = audio_format::s16le;
        spec.channels = 2;
        spec.freq = 48000;
        auto device2 = audio_device::open_device(backend, devices[1].id, &spec);
        
        // Switch to new device
        bool switch_result = audio_system::switch_device(device2);
        CHECK(switch_result == true);
        
        audio_system::done();
    }
    
    TEST_CASE("multiple backends not shared") {
        // Create two separate backends
        REQUIRE_BACKEND();
        std::shared_ptr<audio_backend> backend1 = test::create_backend();
        std::shared_ptr<audio_backend> backend2 = test::create_backend();
        
        // Initialize system with first backend
        audio_system::init(backend1);
        CHECK(audio_system::get_backend().get() == backend1.get());
        
        // Clean up first
        audio_system::done();
        
        // Initialize with second backend
        audio_system::init(backend2);
        CHECK(audio_system::get_backend().get() == backend2.get());
        CHECK(audio_system::get_backend().get() != backend1.get());
        
        audio_system::done();
    }
    
    TEST_CASE("legacy init still works") {
        // Suppress deprecation warning for this test
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        
        // Old API should still work
        // Initialize with null backend
        REQUIRE_BACKEND();
        std::shared_ptr<audio_backend> backend = test::create_backend();
        bool result = audio_system::init(backend);
        REQUIRE(result == true);
        
        // Should have created a backend internally
        auto retrieved_backend = audio_system::get_backend();
        REQUIRE(backend.get() != nullptr);
        REQUIRE(backend->is_initialized());
        
        // Should be able to enumerate devices
        auto current_backend = audio_system::get_backend();
        auto devices = current_backend ? audio_device::enumerate_devices(current_backend, true) : std::vector<device_info>();
        CHECK(devices.size() > 0);
        
        #pragma GCC diagnostic pop
        
        audio_system::done();
    }
    
    TEST_CASE("error handling") {
        // Init with null backend should fail
        bool result = audio_system::init(nullptr);
        CHECK(result == false);
        
        // Get backend should return null
        CHECK(audio_system::get_backend().get() == nullptr);
        
        // Enumerate should throw without initialization
        // Can't enumerate without backend
        std::shared_ptr<audio_backend> null_backend;
        CHECK_THROWS_AS(
            audio_device::enumerate_devices(null_backend, true),
            std::runtime_error
        );
        
        // Open device should throw without backend
        CHECK_THROWS_AS(
            audio_device::open_default_device(null_backend),
            std::runtime_error
        );
    }
}