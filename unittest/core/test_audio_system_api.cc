#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/error.hh>

namespace musac::test {

TEST_SUITE("audio_system_api") {
    // Test fixture to ensure proper initialization/cleanup
    struct audio_system_fixture {
        audio_system_fixture() {
            audio_system::init();
        }
        
        ~audio_system_fixture() {
            audio_system::done();
        }
    };
    
    TEST_CASE_FIXTURE(audio_system_fixture, "device enumeration") {
        SUBCASE("enumerate playback devices") {
            auto devices = audio_system::enumerate_devices(true);
            
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
            auto default_device = audio_system::get_default_device(true);
            
            CHECK(!default_device.name.empty());
            CHECK(!default_device.id.empty());
            CHECK(default_device.is_default == true);
            CHECK(default_device.channels > 0);
            CHECK(default_device.sample_rate > 0);
        }
        
        SUBCASE("default device appears in enumeration") {
            auto default_device = audio_system::get_default_device(true);
            auto all_devices = audio_system::enumerate_devices(true);
            
            bool found_default = false;
            for (const auto& device : all_devices) {
                if (device.id == default_device.id) {
                    found_default = true;
                    CHECK(device.is_default == true);
                    break;
                }
            }
            CHECK(found_default);
        }
    }
    
    TEST_CASE_FIXTURE(audio_system_fixture, "switch_device validation") {
        SUBCASE("switch to valid device returns false (stub)") {
            auto default_device = audio_system::get_default_device(true);
            
            // Currently returns false as it's a stub
            CHECK(audio_system::switch_device(default_device) == false);
        }
        
        SUBCASE("switch to invalid device returns false") {
            device_info invalid_device;
            invalid_device.id = "non_existent_device_id";
            invalid_device.name = "Non-existent Device";
            invalid_device.channels = 2;
            invalid_device.sample_rate = 44100;
            
            CHECK(audio_system::switch_device(invalid_device) == false);
        }
    }
    
    TEST_CASE("audio_system not initialized") {
        // Ensure system is not initialized
        audio_system::done();
        
        SUBCASE("enumerate_devices throws when not initialized") {
            CHECK_THROWS(audio_system::enumerate_devices(true));
        }
        
        SUBCASE("get_default_device throws when not initialized") {
            CHECK_THROWS(audio_system::get_default_device(true));
        }
        
        SUBCASE("switch_device returns false when not initialized") {
            device_info dummy_device;
            CHECK(audio_system::switch_device(dummy_device) == false);
        }
    }
}

} // namespace musac::test