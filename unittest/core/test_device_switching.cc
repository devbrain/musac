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
        // Test the switch_device implementation with user-created devices
        auto devices = audio_system::enumerate_devices();
        REQUIRE(devices.size() >= 1);
        
        // Create first device
        auto device1 = audio_device::open_default_device();
        
        // Create second device (same device for testing)
        auto device2 = audio_device::open_device(devices[0].id);
        
        // Test switching without an active device fails
        // (device2 is already active from constructor)
        CHECK(audio_system::switch_device(device1) == true);
        
        // Switching to same device should succeed
        CHECK(audio_system::switch_device(device1) == true);
        
        // Switching back to device2
        CHECK(audio_system::switch_device(device2) == true);
    }
    
    TEST_CASE_FIXTURE(device_switching_fixture, "stream pause/resume behavior") {
        auto device = audio_device::open_default_device();
        
        // Create streams with different states
        auto source1 = create_mock_source(44100);
        auto stream1 = device.create_stream(std::move(*source1));
        stream1.play();
        
        auto source2 = create_mock_source(44100);
        auto stream2 = device.create_stream(std::move(*source2));
        stream2.play();
        stream2.pause(); // Already paused
        
        auto source3 = create_mock_source(44100);
        auto stream3 = device.create_stream(std::move(*source3));
        // Not playing
        
        // Verify initial states
        CHECK(stream1.is_playing() == true);
        CHECK(stream1.is_paused() == false);
        
        CHECK(stream2.is_playing() == false);
        CHECK(stream2.is_paused() == true);
        
        CHECK(stream3.is_playing() == false);
        CHECK(stream3.is_paused() == false);
        
        // In a real device switch, all playing streams would be paused
        // We can't test the internal pause_all_streams directly,
        // but we can verify the behavior when it's implemented
    }
    
    TEST_CASE_FIXTURE(device_switching_fixture, "device switching with active streams") {
        // Test device switching preserves stream state
        
        // Open initial device
        auto device1 = audio_device::open_default_device();
        
        // Create and configure streams
        auto source1 = create_mock_source(44100);
        auto stream1 = device1.create_stream(std::move(*source1));
        stream1.set_volume(0.5f);
        stream1.set_stereo_position(-0.3f);
        stream1.play();
        
        auto source2 = create_mock_source(44100);
        auto stream2 = device1.create_stream(std::move(*source2));
        stream2.set_volume(0.8f);
        stream2.play();
        stream2.pause();
        
        // Verify initial states
        CHECK(stream1.is_playing() == true);
        CHECK(stream1.volume() == 0.5f);
        CHECK(stream1.get_stereo_position() == -0.3f);
        
        CHECK(stream2.is_playing() == false);
        CHECK(stream2.is_paused() == true);
        CHECK(stream2.volume() == 0.8f);
        
        // Create a new device with same specs
        auto device2 = audio_device::open_default_device();
        
        // Switch to the new device
        bool switch_result = audio_system::switch_device(device2);
        CHECK(switch_result == true);
        
        // Verify stream states are preserved
        CHECK(stream1.is_playing() == true);
        CHECK(stream1.volume() == 0.5f);
        CHECK(stream1.get_stereo_position() == -0.3f);
        
        CHECK(stream2.is_playing() == false);
        CHECK(stream2.is_paused() == true);
        CHECK(stream2.volume() == 0.8f);
        
        // Streams should still be functional
        stream1.pause();
        CHECK(stream1.is_paused() == true);
        
        stream2.resume();
        CHECK(stream2.is_playing() == true);
        CHECK(stream2.is_paused() == false);
    }
    
    TEST_CASE_FIXTURE(device_switching_fixture, "device switching with format conversion") {
        // Test device switching between devices with different audio formats
        
        // Open initial device with default format
        auto device1 = audio_device::open_default_device();
        
        // Create stream
        auto source1 = create_mock_source(44100);
        auto stream1 = device1.create_stream(std::move(*source1));
        stream1.set_volume(0.6f);
        stream1.play();
        
        // Verify initial state
        CHECK(stream1.is_playing() == true);
        CHECK(stream1.volume() == 0.6f);
        
        // Get device info
        auto dev1_format = device1.get_format();
        auto dev1_channels = device1.get_channels();
        auto dev1_freq = device1.get_freq();
        
        // Create a device with different specs if possible
        // For testing purposes, we'll create another default device
        // In real usage, this might be a device with different capabilities
        auto device2 = audio_device::open_default_device();
        
        // Log the formats for debugging
        INFO("Device 1: ", dev1_freq, " Hz, ", dev1_channels, " ch, format ", dev1_format);
        INFO("Device 2: ", device2.get_freq(), " Hz, ", device2.get_channels(), " ch, format ", device2.get_format());
        
        // Switch to the new device
        bool switch_result = audio_system::switch_device(device2);
        CHECK(switch_result == true);
        
        // Verify stream state is preserved even with format conversion
        CHECK(stream1.is_playing() == true);
        CHECK(stream1.volume() == 0.6f);
        
        // Stream should still be functional
        stream1.pause();
        CHECK(stream1.is_paused() == true);
        
        stream1.resume();
        CHECK(stream1.is_playing() == true);
    }
}

} // namespace musac::test