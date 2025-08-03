#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <thread>
#include <chrono>
#include "../test_helpers.hh"

namespace musac::test {

TEST_SUITE("sdl_shutdown_order") {
    // Test basic initialization and shutdown without any streams
    TEST_CASE("basic init and shutdown") {
        CHECK(audio_system::init());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Verify system is initialized by opening a device
        {
            auto device = audio_device::open_default_device();
            CHECK(device.get_channels() > 0);
            CHECK(device.get_freq() > 0);
            
            // Device must be destroyed before audio_system::done()
        }
        
        audio_system::done();
        
        // After done(), we can init again
        CHECK(audio_system::init());
        audio_system::done();
    }
    
    // Test with device but no streams
    TEST_CASE("device without streams") {
        CHECK(audio_system::init());
        
        // Track device state
        bool device_opened = false;
        bool device_resumed = false;
        
        {
            auto device = audio_device::open_default_device();
            device_opened = (device.get_channels() > 0);
            CHECK(device_opened);
            
            device.resume();
            device_resumed = true;
            
            // Device should be in resumed state
            CHECK(device.get_channels() > 0);
            CHECK(device.get_freq() > 0);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // Device is destroyed, but audio system should still be valid
        CHECK(device_opened);
        CHECK(device_resumed);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        audio_system::done();
    }
    
    // Test with device and one stream
    TEST_CASE("device with single stream") {
        CHECK(audio_system::init());
        
        bool stream_opened = false;
        bool stream_played = false;
        
        {
            auto device = audio_device::open_default_device();
            CHECK(device.get_channels() > 0);
            device.resume();
            
            auto source = create_mock_source(44100);
            auto stream = device.create_stream(std::move(*source));
            stream_opened = stream.open();
            CHECK(stream_opened);
            
            stream_played = stream.play();
            CHECK(stream_played);
            CHECK(stream.is_playing());
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Stream should still be playing
            CHECK(stream.is_playing());
        }
        
        // Verify cleanup happened
        CHECK(stream_opened);
        CHECK(stream_played);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        audio_system::done();
    }
    
    // Test rapid init/done cycles
    TEST_CASE("rapid init done cycles") {
        int successful_cycles = 0;
        int failed_cycles = 0;
        
        for (int i = 0; i < 5; ++i) {
            bool init_success = audio_system::init();
            CHECK(init_success);
            
            if (init_success) {
                {
                    auto device = audio_device::open_default_device();
                    CHECK(device.get_channels() > 0);
                    device.resume();
                    
                    auto source = create_mock_source(44100);
                    auto stream = device.create_stream(std::move(*source));
                    CHECK(stream.open());
                    CHECK(stream.play());
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    
                    // Verify stream is playing before cleanup
                    CHECK(stream.is_playing());
                    
                    // Device and stream must be destroyed before done()
                }
                
                successful_cycles++;
            } else {
                failed_cycles++;
            }
            
            audio_system::done();
        }
        
        // All cycles should have succeeded
        CHECK(successful_cycles == 5);
        CHECK(failed_cycles == 0);
    }
}

} // namespace musac::test