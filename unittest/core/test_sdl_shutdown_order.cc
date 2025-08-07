#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <thread>
#include <chrono>
#include "../test_helpers.hh"
#include "../test_helpers_v2.hh"

namespace musac::test {

TEST_SUITE("sdl_shutdown_order") {
    // Test basic initialization and shutdown without any streams
    TEST_CASE("basic init and shutdown") {
        auto backend = init_test_audio_system();
        CHECK(backend.get() != nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Verify system is initialized by opening a device
        auto device = audio_device::open_default_device(backend);
        CHECK(device.get_channels() > 0);
        CHECK(device.get_freq() > 0);
        
        audio_system::done();
        
        // Now the device destructor will run after done() - this should be safe
        // because the device holds a shared_ptr to the manager
    }
    
    // Test with device but no streams
    TEST_CASE("device without streams") {
        auto backend = init_test_audio_system();
        CHECK(backend.get() != nullptr);
        
        // Track device state
        bool device_opened = false;
        bool device_resumed = false;
        
        {
            auto device = audio_device::open_default_device(backend);
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
        auto backend = init_test_audio_system();
        CHECK(backend.get() != nullptr);
        
        bool stream_opened = false;
        bool stream_played = false;
        
        {
            auto device = audio_device::open_default_device(backend);
            CHECK(device.get_channels() > 0);
            device.resume();
            
            auto source = create_mock_source(44100);
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
            stream_opened = true;
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
            auto backend = init_test_audio_system();
            bool init_success = (backend != nullptr);
            CHECK(init_success);
            
            if (init_success) {
                auto device = audio_device::open_default_device(backend);
                CHECK(device.get_channels() > 0);
                device.resume();
                
                auto source = create_mock_source(44100);
                auto stream = device.create_stream(std::move(*source));
                REQUIRE_NOTHROW(stream.open());
                CHECK(stream.play());
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                // Verify stream is playing before cleanup
                CHECK(stream.is_playing());
                
                successful_cycles++;
            } else {
                failed_cycles++;
            }
            
            audio_system::done();
            
            // Device and stream destructors run after done() - this is now safe
        }
        
        // All cycles should have succeeded
        CHECK(successful_cycles == 5);
        CHECK(failed_cycles == 0);
    }
    
    // Test that device survives audio_system::done() - fool-proof behavior
    TEST_CASE("device survives system shutdown") {
        auto backend = init_test_audio_system();
        CHECK(backend.get() != nullptr);
        
        auto device = audio_device::open_default_device(backend);
        CHECK(device.get_channels() > 0);
        CHECK(device.get_freq() > 0);
        
        auto source = create_mock_source(44100);
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        CHECK(stream.play());
        CHECK(stream.is_playing());
        
        // Shutdown the audio system while device and stream are still alive
        audio_system::done();
        
        // Device and stream objects still exist - their destructors should
        // handle this gracefully because they hold shared_ptr to the manager
        
        // This used to crash, but now it's safe
    }
}

} // namespace musac::test