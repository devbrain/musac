#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <thread>
#include <chrono>
#include "../test_helpers.hh"

namespace musac::test {

TEST_SUITE("device_switching") {
    // Test fixture to ensure proper initialization/cleanup
    struct device_switching_fixture {
        device_switching_fixture() {
            audio_system::init();
        }
        
        ~device_switching_fixture() {
            audio_system::done();
        }
    };
    
    TEST_CASE_FIXTURE(device_switching_fixture, "device enumeration") {
        // Test device enumeration through public API
        auto devices = audio_system::enumerate_devices();
        
        CHECK(devices.size() >= 1); // At least one device
        
        // Find default device
        bool found_default = false;
        for (const auto& dev : devices) {
            if (dev.is_default) {
                found_default = true;
                break;
            }
        }
        CHECK(found_default);
        
        // Test get_default_device
        auto default_device = audio_system::get_default_device();
        CHECK(!default_device.id.empty());
        CHECK(default_device.is_default);
    }
    
    TEST_CASE_FIXTURE(device_switching_fixture, "stream public API state preservation") {
        auto device = audio_device::open_default_device();
        auto source = create_mock_source(44100);
        auto stream = device.create_stream(std::move(*source));
        
        // Set initial state using public API
        stream.open();
        stream.set_volume(0.8f);
        stream.set_stereo_position(0.5f);
        stream.play();
        
        // Verify initial state through public API
        CHECK(stream.volume() == 0.8f);
        CHECK(stream.get_stereo_position() == 0.5f);
        CHECK(stream.is_playing() == true);
        CHECK(stream.is_paused() == false);
        
        SUBCASE("volume and position can be changed") {
            stream.set_volume(0.3f);
            stream.set_stereo_position(-0.5f);
            
            CHECK(stream.volume() == 0.3f);
            CHECK(stream.get_stereo_position() == -0.5f);
        }
        
        SUBCASE("play state can be changed") {
            stream.pause();
            CHECK(stream.is_paused() == true);
            CHECK(stream.is_playing() == false);
            
            stream.resume();
            CHECK(stream.is_paused() == false);
            CHECK(stream.is_playing() == true);
        }
        
        SUBCASE("stream state persists across operations") {
            // This tests that stream state is properly managed
            // which is critical for device switching
            stream.set_volume(0.5f);
            stream.pause();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // State should persist
            CHECK(stream.volume() == 0.5f);
            CHECK(stream.is_paused() == true);
            
            stream.resume();
            CHECK(stream.is_playing() == true);
            CHECK(stream.volume() == 0.5f); // Volume unchanged
        }
    }
    
    TEST_CASE_FIXTURE(device_switching_fixture, "multiple streams state management") {
        auto device = audio_device::open_default_device();
        
        // Create multiple streams
        std::vector<audio_stream> streams;
        for (int i = 0; i < 3; ++i) {
            auto source = create_mock_source(44100);
            streams.push_back(device.create_stream(std::move(*source)));
        }
        
        // Set different states
        streams[0].set_volume(0.5f);
        streams[0].play();
        
        streams[1].set_volume(0.7f);
        streams[1].set_stereo_position(-0.5f);
        streams[1].play();
        streams[1].pause();
        
        streams[2].set_volume(0.9f);
        // streams[2] not playing
        
        // Verify states are independent
        CHECK(streams[0].is_playing() == true);
        CHECK(streams[0].is_paused() == false);
        CHECK(streams[0].volume() == 0.5f);
        
        CHECK(streams[1].is_playing() == false);
        CHECK(streams[1].is_paused() == true);
        CHECK(streams[1].volume() == 0.7f);
        CHECK(streams[1].get_stereo_position() == -0.5f);
        
        CHECK(streams[2].is_playing() == false);
        CHECK(streams[2].is_paused() == false);
        CHECK(streams[2].volume() == 0.9f);
    }
    
    TEST_CASE_FIXTURE(device_switching_fixture, "device switching API validation") {
        // Test the switch_device stub
        auto devices = audio_system::enumerate_devices();
        REQUIRE(devices.size() >= 1);
        
        // Currently returns false (stub)
        CHECK(audio_system::switch_device(devices[0]) == false);
        
        // Test with invalid device
        device_info invalid_device;
        invalid_device.id = "non_existent_device";
        CHECK(audio_system::switch_device(invalid_device) == false);
    }
}

} // namespace musac::test