#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <thread>
#include <chrono>
#include <stdexcept>
#include "../test_helpers.hh"

namespace musac::test {

TEST_SUITE("audio_device") {
    struct audio_test_fixture {
        audio_test_fixture() {
            audio_system::init();
        }
        
        ~audio_test_fixture() {
            audio_system::done();
        }
    };
    
    TEST_CASE_FIXTURE(audio_test_fixture, "device enumeration") {
        auto devices = audio_device::enumerate_devices(true);
        
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
        auto device = audio_device::open_default_device();
        
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
        
        auto device = audio_device::open_default_device(&desired);
        
        // Device should respect requested spec or provide compatible alternative
        CHECK(device.get_channels() > 0);
        CHECK(device.get_freq() > 0);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "device pause/resume") {
        auto device = audio_device::open_default_device();
        
        CHECK_FALSE(device.is_paused());
        
        CHECK(device.pause());
        CHECK(device.is_paused());
        
        CHECK(device.resume());
        CHECK_FALSE(device.is_paused());
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "device gain control") {
        auto device = audio_device::open_default_device();
        
        float initial_gain = device.get_gain();
        CHECK(initial_gain >= 0.0f);
        CHECK(initial_gain <= 1.0f);
        
        device.set_gain(0.5f);
        CHECK(device.get_gain() == doctest::Approx(0.5f).epsilon(0.01));
        
        device.set_gain(1.0f);
        CHECK(device.get_gain() == doctest::Approx(1.0f).epsilon(0.01));
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "multiple device instances") {
        // Only one device can be active at a time
        auto device1 = audio_device::open_default_device();
        CHECK(device1.get_channels() > 0);
        
        // Trying to create a second device should throw
        CHECK_THROWS_AS(
            audio_device::open_default_device(),
            std::runtime_error
        );
        
        // Operations on the device should work
        device1.pause();
        CHECK(device1.is_paused());
        device1.resume();
        CHECK_FALSE(device1.is_paused());
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "device move semantics") {
        auto device1 = audio_device::open_default_device();
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
            auto device1 = audio_device::open_default_device();
            
            // Create stream from device
            auto source1 = mock_audio_source::create();
            auto stream1 = device1.create_stream(std::move(*source1));
            
            // Device and stream will be destroyed when going out of scope
        }
        
        // Now we can create a new device
        {
            auto device2 = audio_device::open_default_device();
            
            // Create stream from new device
            auto source2 = mock_audio_source::create();
            auto stream2 = device2.create_stream(std::move(*source2));
            
            // This should work fine
        }
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "open non-existent device") {
        // Try to open a device with invalid ID
        CHECK_THROWS(audio_device::open_device("non_existent_device_id_12345"));
    }
}

} // namespace musac::test