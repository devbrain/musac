#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/error.hh>
#include "../test_helpers_v2.hh"

namespace musac::test {

TEST_SUITE("audio_system_api") {
    // Test fixture to ensure proper initialization/cleanup
    struct audio_system_fixture {
        std::shared_ptr<audio_backend_v2> backend;
        
        audio_system_fixture() {
            backend = init_test_audio_system();
        }
        
        ~audio_system_fixture() {
            audio_system::done();
        }
    };
    
    TEST_CASE_FIXTURE(audio_system_fixture, "device enumeration") {
        SUBCASE("enumerate playback devices") {
            auto devices = audio_device::enumerate_devices(backend, true);
            
            // Should have at least one device (default)
            CHECK(devices.size() >= 1);
            
            // Check device info fields are populated
            for (const auto& device : devices) {
                CHECK(!device.name.empty());
                CHECK(!device.id.empty());
                CHECK(device.channels > 0);
                CHECK(device.sample_rate > 0);
            }
            
            // Should have at least one default device
            bool has_default = false;
            for (const auto& device : devices) {
                if (device.is_default) {
                    has_default = true;
                    break;
                }
            }
            CHECK(has_default);
        }
        
        SUBCASE("get default device") {
            auto devices = audio_device::enumerate_devices(backend, true);
            
            // Find the default device
            device_info default_device;
            bool found = false;
            for (const auto& dev : devices) {
                if (dev.is_default) {
                    default_device = dev;
                    found = true;
                    break;
                }
            }
            
            CHECK(found);
            CHECK(!default_device.name.empty());
            CHECK(!default_device.id.empty());
            CHECK(default_device.is_default == true);
            CHECK(default_device.channels > 0);
            CHECK(default_device.sample_rate > 0);
        }
        
        SUBCASE("default device appears in enumeration") {
            auto all_devices = audio_device::enumerate_devices(backend, true);
            
            // Find the default device
            device_info default_device;
            bool found_default = false;
            for (const auto& device : all_devices) {
                if (device.is_default) {
                    default_device = device;
                    found_default = true;
                    break;
                }
            }
            CHECK(found_default);
            
            // Verify it's marked as default in the enumeration
            for (const auto& device : all_devices) {
                if (device.id == default_device.id) {
                    CHECK(device.is_default == true);
                    break;
                }
            }
        }
    }
    
    TEST_CASE_FIXTURE(audio_system_fixture, "switch_device validation") {
        SUBCASE("switch device requires audio_device object") {
            // Create a device to switch to
            auto device = audio_device::open_default_device(backend);
            
            // Switching to the device should succeed
            CHECK(audio_system::switch_device(device) == true);
        }
        
        SUBCASE("switch to same device succeeds") {
            auto device1 = audio_device::open_default_device(backend);
            CHECK(audio_system::switch_device(device1) == true);
            
            // Switching to same device again should succeed
            CHECK(audio_system::switch_device(device1) == true);
        }
    }
    
    TEST_CASE("audio_system not initialized") {
        // Ensure system is not initialized
        audio_system::done();
        
        SUBCASE("enumerate_devices throws when not initialized") {
            // Can't enumerate without a backend
            std::shared_ptr<audio_backend_v2> null_backend;
            CHECK_THROWS(audio_device::enumerate_devices(null_backend, true));
        }
        
        SUBCASE("open_default_device throws when not initialized") {
            // Can't open device without a backend
            std::shared_ptr<audio_backend_v2> null_backend;
            CHECK_THROWS(audio_device::open_default_device(null_backend));
        }
        
        SUBCASE("switch_device returns false when not initialized") {
            // Since switch_device now takes audio_device, we can't test this without init
            // Creating an audio_device would fail without initialization
            // So this test is no longer valid
        }
    }
}

} // namespace musac::test