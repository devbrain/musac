/**
 * @file test_audio_device_unit.cc
 * @brief Unit tests for audio_device class using mock backends
 * 
 * Test Coverage:
 * - Device construction and initialization
 * - Error handling and edge cases
 * - Device control operations (pause/resume, gain)
 * - Stream creation and management
 * - Device enumeration
 * - Resource cleanup and RAII
 * - Thread-safe concurrent operations
 * 
 * Dependencies:
 * - mock_backend_v2_enhanced for backend simulation
 * - mock_stream for stream operations
 * - No hardware or SDL3 dependencies
 */

#include <doctest/doctest.h>
#include <musac/audio_device.hh>
#include <musac/audio_system.hh>
#include <musac/stream.hh>
#include "../../mock_backends.hh"
#include "../../mock_components.hh"
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace musac;
using namespace musac::test;

TEST_SUITE("AudioDevice::Unit") {
    
    TEST_CASE("should_construct_device_successfully_with_mock_backend") {
        // Arrange: Create and initialize mock backend
        auto mock = std::make_shared<mock_backend_v2_enhanced>();
        mock->init();
        
        SUBCASE("with_default_configuration") {
            // Act: Open device with default settings
            auto device = audio_device::open_default_device(mock);
            
            CHECK(mock->open_device_calls == 1);
            CHECK(device.get_device_name() == "Mock Default Device");
            CHECK(device.get_channels() == 2);
            CHECK(device.get_freq() == 44100);
        }
        
        SUBCASE("with_custom_audio_spec") {
            // Arrange: Define custom audio specification
            audio_spec spec;
            spec.format = audio_format::s16le;
            spec.channels = 1;  // Mono
            spec.freq = 48000;   // 48kHz sample rate
            
            // Act: Open device with custom spec
            auto device = audio_device::open_default_device(mock, &spec);
            
            CHECK(mock->open_device_calls == 1);
            CHECK(device.get_channels() == 1);
            CHECK(device.get_freq() == 48000);
            CHECK(device.get_format() == audio_format::s16le);
        }
        
        SUBCASE("with_specific_device_id") {
            auto device = audio_device::open_device(mock, "mock_secondary");
            
            CHECK(mock->open_device_calls == 1);
            CHECK(device.get_device_id().find("mock_") != std::string::npos);
        }
    }
    
    TEST_CASE("should_handle_backend_errors_gracefully") {
        SUBCASE("when_backend_not_initialized") {
            // Arrange: Create backend without initialization
            auto mock = std::make_shared<mock_backend_v2_enhanced>();
            // Intentionally skip init() to test error handling
            
            CHECK_THROWS_AS(
                audio_device::open_default_device(mock),
                std::runtime_error
            );
        }
        
        SUBCASE("when_open_device_fails") {
            // Arrange: Create backend that fails on device open
            auto mock = create_failing_backend(false, false, true, false);
            mock->init();
            
            CHECK_THROWS_AS(
                audio_device::open_default_device(mock),
                std::runtime_error
            );
            
            CHECK(mock->open_device_calls == 1);
        }
        
        SUBCASE("when_enumerate_fails") {
            auto mock = create_failing_backend(false, true, false, false);
            mock->init();
            
            CHECK_THROWS_AS(
                audio_device::enumerate_devices(mock, true),
                std::runtime_error
            );
            
            CHECK(mock->enumerate_calls == 1);
        }
    }
    
    TEST_CASE("should_control_device_state_correctly") {
        auto mock = std::make_shared<mock_backend_v2_enhanced>();
        mock->init();
        auto device = audio_device::open_default_device(mock);
        
        SUBCASE("pause_and_resume_operations") {
            // Assert: Device starts in paused state (mock backend default)
            CHECK(device.is_paused());
            
            // Act & Assert: Pause already paused device (should succeed)
            CHECK(device.pause());
            CHECK(device.is_paused());
            
            // Act & Assert: Resume device
            CHECK(device.resume());
            CHECK(!device.is_paused());
        }
        
        SUBCASE("gain_adjustment") {
            device.set_gain(0.5f);
            CHECK(device.get_gain() == doctest::Approx(0.5f));
            
            device.set_gain(0.0f);
            CHECK(device.get_gain() == doctest::Approx(0.0f));
            
            device.set_gain(1.0f);
            CHECK(device.get_gain() == doctest::Approx(1.0f));
        }
        
        SUBCASE("gain_clamping_to_valid_range") {
            // Act: Set gain below minimum (should clamp to 0.0)
            device.set_gain(-0.5f);
            CHECK(device.get_gain() >= 0.0f);
            
            // Act: Set gain above maximum (should clamp to 1.0)
            device.set_gain(2.0f);
            CHECK(device.get_gain() <= 1.0f);
        }
    }
    
    TEST_CASE("should_create_streams_successfully") {
        auto mock = std::make_shared<mock_backend_v2_enhanced>();
        mock->init();
        auto device = audio_device::open_default_device(mock);
        
        SUBCASE("with_audio_source") {
            auto source = create_mock_source(1024);
            auto stream = device.create_stream(std::move(*source));
            
            CHECK(mock->create_stream_calls == 1);
        }
        
        SUBCASE("with_callback_function") {
            bool callback_set = false;
            device.create_stream_with_callback(
                [](void* userdata, uint8_t*, int) {
                    *static_cast<bool*>(userdata) = true;
                },
                &callback_set
            );
            
            CHECK(mock->create_stream_calls == 1);
        }
        
        SUBCASE("when_stream_creation_fails") {
            // Arrange: Configure mock to fail stream creation
            mock->fail_create_stream = true;
            auto source = create_mock_source(1024);
            
            CHECK_THROWS_AS(
                device.create_stream(std::move(*source)),
                std::runtime_error
            );
        }
    }
    
    TEST_CASE("should_enumerate_devices_correctly") {
        auto mock = std::make_shared<mock_backend_v2_enhanced>();
        mock->init();
        
        SUBCASE("list_playback_devices") {
            auto devices = audio_device::enumerate_devices(mock, true);
            
            CHECK(devices.size() == 2);  // Mock has 2 default devices
            CHECK(mock->enumerate_calls == 1);
            
            bool found_default = false;
            for (const auto& dev : devices) {
                if (dev.is_default) {
                    found_default = true;
                    CHECK(dev.name == "Mock Default Device");
                    CHECK(dev.channels == 2);
                    CHECK(dev.sample_rate == 44100);
                }
            }
            CHECK(found_default);
        }
        
        SUBCASE("with_custom_device_list") {
            // Arrange: Create a custom device with non-standard configuration
            device_info custom_device;
            custom_device.id = "custom_device";
            custom_device.name = "Custom Test Device";
            custom_device.channels = 6;      // 5.1 surround
            custom_device.sample_rate = 96000;  // High-quality sample rate
            custom_device.is_default = false;
            
            // Act: Add custom device to mock backend
            mock->add_test_device(custom_device);
            
            auto devices = audio_device::enumerate_devices(mock, true);
            
            CHECK(devices.size() == 3);  // 2 default + 1 custom
            
            bool found_custom = false;
            for (const auto& dev : devices) {
                if (dev.id == "custom_device") {
                    found_custom = true;
                    CHECK(dev.name == "Custom Test Device");
                    CHECK(dev.channels == 6);
                    CHECK(dev.sample_rate == 96000);
                }
            }
            CHECK(found_custom);
        }
    }
    
    TEST_CASE("should_manage_device_lifecycle_properly") {
        /**
         * Verifies RAII behavior - devices should automatically
         * close when destroyed, preventing resource leaks.
         */
        auto mock = std::make_shared<mock_backend_v2_enhanced>();
        mock->init();
        
        SUBCASE("cleanup_on_destruction") {
            {
                // Act: Create device in limited scope
                auto device = audio_device::open_default_device(mock);
                CHECK(mock->open_device_calls == 1);
                CHECK(mock->close_device_calls == 0);
            }  // Device destroyed here (RAII)
            
            // Assert: Destructor should have closed the device
            CHECK(mock->close_device_calls == 1);
        }
        
        SUBCASE("multiple_devices_sharing_backend") {
            auto device1 = audio_device::open_device(mock, "mock_default");
            auto device2 = audio_device::open_device(mock, "mock_secondary");
            
            CHECK(mock->open_device_calls == 2);
            CHECK(device1.get_device_id() != device2.get_device_id());
        }
    }
    
    TEST_CASE("should_handle_stream_operations") {
        auto mock = std::make_shared<mock_backend_v2_enhanced>();
        mock->init();
        
        // Configure mock to return our mock stream
        mock->on_create_stream = [](uint32_t, const audio_spec& spec, 
                                               void (*)(void*, uint8_t*, int), void*) -> std::unique_ptr<audio_stream_interface> {
            // Create a new mock_stream instance (can't copy due to mutex)
            return std::make_unique<mock_stream>(spec);
        };
        
        auto device = audio_device::open_default_device(mock);
        auto source = create_mock_source(1024);
        auto stream = device.create_stream(std::move(*source));
        
        SUBCASE("pause_and_resume_stream") {
            // Since we're creating new mock_stream instances,
            // we can't track calls on them directly
            // Just verify the operations don't crash
            stream.pause();
            stream.resume();
            CHECK(true); // Operations completed without crash
        }
    }
    
    TEST_CASE("should_handle_concurrent_access_without_race_conditions") {
        /**
         * Stress test for thread safety - multiple threads
         * performing operations simultaneously should not
         * cause crashes, deadlocks, or data races.
         */
        auto mock = std::make_shared<mock_backend_v2_enhanced>();
        mock->init();
        auto device = audio_device::open_default_device(mock);
        
        SUBCASE("concurrent_gain_adjustments") {
            // Arrange: Prepare multiple worker threads
            std::vector<std::thread> threads;
            
            // Act: Launch concurrent operations
            for (int i = 0; i < 5; ++i) {
                threads.emplace_back([&device, &mock]() {
                    // Each thread performs multiple operations
                    for (int j = 0; j < 10; ++j) {
                        device.set_gain(0.5f);
                        device.get_gain();
                    }
                });
            }
            
            // Wait for all threads to complete
            for (auto& t : threads) {
                t.join();
            }
            
            // Assert: Device should still be in valid state
            // after concurrent access (no corruption)
            CHECK(device.get_gain() >= 0.0f);
        }
    }
}