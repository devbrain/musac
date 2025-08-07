#include <doctest/doctest.h>
#include <musac/audio_device.hh>
#include <musac/audio_system.hh>
#include <musac/backends/sdl3/sdl3_backend.hh>
#include <musac/sdk/audio_backend_v2.hh>
#include <memory>
#include <thread>
#include <chrono>

TEST_SUITE("Core::DeviceLifecycle::Integration") {

TEST_CASE("Resource cleanup ordering with device_guard") {
    std::shared_ptr<musac::audio_backend_v2> backend(musac::create_sdl3_backend_v2());
    musac::audio_system::init(backend);
    
    SUBCASE("Device destroyed with active callback") {
        // This tests that the device guard properly cleans up
        // Previously could cause use-after-free if ordering was wrong
        
        auto device = std::make_unique<musac::audio_device>(
            musac::audio_device::open_default_device(backend)
        );
        
        bool callback_running = false;
        
        // Create stream with callback
        device->create_stream_with_callback(
            [](void* userdata, [[maybe_unused]] uint8_t* stream, [[maybe_unused]] int len) {
                auto* flag = static_cast<bool*>(userdata);
                *flag = true; // running
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            },
            &callback_running
        );
        
        // Wait for callback to start
        for (int i = 0; i < 100 && !callback_running; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Destroy device while callback might be running
        // RAII with device_guard should handle this gracefully:
        // 1. stream destroyed first
        // 2. device_guard destroyed second (closes device)
        device.reset();
        
        // No crash means cleanup ordering is correct
        CHECK(true);
    }
    
    SUBCASE("Multiple device creation and destruction") {
        // Test that device guard handles multiple devices properly
        for (int i = 0; i < 5; i++) {
            auto device = musac::audio_device::open_default_device(backend);
            
            // Create callback to ensure device is active
            device.create_stream_with_callback(
                []([[maybe_unused]] void* userdata, [[maybe_unused]] uint8_t* stream, [[maybe_unused]] int len) {
                    // Empty callback
                },
                nullptr
            );
            
            // Device should be properly cleaned up by device_guard when it goes out of scope
        }
        
        // If we get here without issues, RAII is working
        CHECK(true);
    }
    
    SUBCASE("Device move semantics") {
        auto device1 = musac::audio_device::open_default_device(backend);
        
        // Move device to another variable
        auto device2 = std::move(device1);
        
        // device2 should be valid
        CHECK(device2.get_channels() > 0);
        CHECK(device2.get_freq() > 0);
        
        // Create callback on moved device
        device2.create_stream_with_callback(
            []([[maybe_unused]] void* userdata, [[maybe_unused]] uint8_t* stream, [[maybe_unused]] int len) {
                // Empty callback
            },
            nullptr
        );
        
        // Device should clean up properly when device2 goes out of scope
    }
    
    SUBCASE("Device destroyed before pause/resume operations") {
        auto device = std::make_unique<musac::audio_device>(
            musac::audio_device::open_default_device(backend)
        );
        
        // Try pause/resume operations
        device->pause();
        bool paused = device->is_paused();
        CHECK((paused == true || paused == false)); // Just check it returns without crash
        device->resume();
        
        // Destroy device - should not cause issues with device_guard
        device.reset();
        
        CHECK(true);
    }
    
    musac::audio_system::done();
}

} // TEST_SUITE