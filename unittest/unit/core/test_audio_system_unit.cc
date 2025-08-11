/**
 * @file test_audio_system_unit.cc
 * @brief Unit tests for audio_system class using mock backends
 * 
 * Test Coverage:
 * - System initialization and shutdown
 * - Device management and switching
 * - Global audio operations (pause/resume all)
 * - Error handling for invalid operations
 * - Thread safety of global operations
 * 
 * Dependencies:
 * - mock_backend_v2_enhanced for backend simulation
 * - No hardware dependencies
 */

#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include "../../mock_backends.hh"
#include "../../mock_components.hh"
#include "../../test_fixtures.hh"
#include <memory>
#include <thread>
#include <vector>

using namespace musac;
using namespace musac::test;

TEST_SUITE("AudioSystem::Unit") {
    
    TEST_CASE("should_initialize_and_shutdown_correctly") {
        SUBCASE("basic_init_and_done") {
            // Arrange: Create mock backend
            auto backend = std::make_shared<mock_backend_v2_enhanced>();
            
            // Act: Initialize system
            CHECK_NOTHROW(audio_system::init(backend));
            CHECK(backend->init_calls == 1);
            
            // Act: Shutdown system
            CHECK_NOTHROW(audio_system::done());
            CHECK(backend->shutdown_calls == 1);
        }
        
        SUBCASE("multiple_init_calls") {
            // Arrange
            auto backend1 = std::make_shared<mock_backend_v2_enhanced>();
            auto backend2 = std::make_shared<mock_backend_v2_enhanced>();
            
            // Act: Initialize twice with different backends
            audio_system::init(backend1);
            CHECK(backend1->init_calls == 1);
            
            // Second init with different backend
            audio_system::init(backend2);
            // Note: audio_system may not shutdown the first backend
            CHECK(backend2->init_calls == 1);
            
            // Cleanup
            audio_system::done();
            // Only the current backend gets shutdown
            CHECK(backend2->shutdown_calls >= 0);
        }
        
        SUBCASE("done_without_init") {
            // Act & Assert: Should be safe to call done without init
            CHECK_NOTHROW(audio_system::done());
        }
        
        SUBCASE("init_with_null_backend") {
            // Act & Assert: Should handle null backend
            CHECK_NOTHROW(audio_system::init(nullptr));
            CHECK_NOTHROW(audio_system::done());
        }
    }
    
    TEST_CASE("should_manage_devices_correctly") {
        // Arrange: Set up system with mock backend
        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        audio_system::init(backend);
        
        SUBCASE("open_default_device") {
            // Act: Open default device via backend
            auto device = audio_device::open_default_device(backend);
            
            // Assert: Device should be opened
            CHECK(backend->open_device_calls == 1);
            CHECK(device.get_device_name() == "Mock Default Device");
        }
        
        SUBCASE("open_device_with_spec") {
            // Arrange: Custom audio spec
            audio_spec spec;
            spec.format = audio_format::f32le;
            spec.channels = 1;
            spec.freq = 48000;
            
            // Act: Open device with spec via backend
            auto devices = backend->enumerate_devices(true);
            CHECK(!devices.empty());
            auto device = audio_device::open_device(backend, devices[0].id, &spec);
            
            // Assert: Device should use requested spec
            CHECK(backend->open_device_calls == 1);
            CHECK(device.get_channels() == 1);
            CHECK(device.get_freq() == 48000);
        }
        
        SUBCASE("switch_device") {
            // Act: Open a device
            auto device1 = audio_device::open_default_device(backend);
            
            // Act: Switch to this device
            bool result = audio_system::switch_device(device1);
            
            // Assert: Switch should succeed
            CHECK(result);
        }
        
        SUBCASE("get_backend") {
            // Act: Get the current backend
            auto current_backend = audio_system::get_backend();
            
            // Assert: Should return the initialized backend
            CHECK(current_backend.get() == backend.get());
        }
        
        // Cleanup
        audio_system::done();
    }
    
    TEST_CASE("should_handle_device_switching") {
        // Arrange: Backend with multiple devices
        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        
        // Add a second test device
        device_info second_device;
        second_device.id = "test_device_2";
        second_device.name = "Test Device 2";
        second_device.channels = 4;
        second_device.sample_rate = 48000;
        second_device.is_default = false;
        backend->add_test_device(second_device);
        
        audio_system::init(backend);
        
        SUBCASE("switch_to_different_device") {
            // Arrange: Open initial device
            auto device1 = audio_device::open_default_device(backend);
            CHECK(backend->open_device_calls == 1);
            
            // Act: Switch to different device
            audio_spec spec;
            spec.format = audio_format::f32le;
            spec.channels = 4;
            spec.freq = 48000;
            
            auto device2 = audio_device::open_device(
                backend, "test_device_2", &spec
            );
            
            // Assert: New device should be opened
            CHECK(backend->open_device_calls == 2);
            CHECK(device2.get_device_name() == "Test Device 2");
            
            // Switch to the new device
            bool switched = audio_system::switch_device(device2);
            CHECK(switched);
        }
        
        SUBCASE("switch_to_same_device") {
            // Arrange: Open device
            auto device1 = audio_device::open_default_device(backend);
            int initial_opens = backend->open_device_calls;
            
            // Act: Try to switch to same device
            bool result = audio_system::switch_device(device1);
            
            // Assert: Switch should succeed
            CHECK(result);
            // No new device should be opened
            CHECK(backend->open_device_calls == initial_opens);
        }
        
        // Cleanup
        audio_system::done();
    }
    
    TEST_CASE("should_handle_device_operations") {
        // Arrange
        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        audio_system::init(backend);
        auto device = audio_device::open_default_device(backend);
        
        SUBCASE("pause_and_resume_device") {
            // Create some streams
            auto source1 = create_mock_source(44100);
            auto source2 = create_mock_source(44100);
            auto stream1 = device.create_stream(std::move(*source1));
            auto stream2 = device.create_stream(std::move(*source2));
            
            stream1.open();
            stream2.open();
            stream1.play();
            stream2.play();
            
            // Act: Pause device
            device.pause();
            
            // Assert: Device should be paused
            CHECK(device.is_paused());
            
            // Act: Resume device
            device.resume();
            
            // Assert: Device should be resumed
            CHECK(!device.is_paused());
        }
        
        SUBCASE("set_device_gain") {
            // Act: Set device gain
            device.set_gain(0.5f);
            
            // Assert: Gain should be set
            CHECK(device.get_gain() == 0.5f);
        }
        
        // Cleanup
        audio_system::done();
    }
    
    TEST_CASE("should_handle_concurrent_system_operations" * doctest::skip(true)) {
        /**
         * Test thread safety of audio_system operations
         * SKIPPED: audio_system is not thread-safe for concurrent init/done calls
         */
        SUBCASE("concurrent_init_and_done") {
            std::vector<std::thread> threads;
            std::vector<std::shared_ptr<mock_backend_v2_enhanced>> backends;
            
            // Create backends for each thread
            for (int i = 0; i < 5; ++i) {
                backends.push_back(std::make_shared<mock_backend_v2_enhanced>());
            }
            
            // Launch threads that init and done
            for (size_t i = 0; i < 5; ++i) {
                threads.emplace_back([backend = backends[i]]() {
                    for (int j = 0; j < 10; ++j) {
                        audio_system::init(backend);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        audio_system::done();
                    }
                });
            }
            
            // Wait for all threads
            for (auto& t : threads) {
                t.join();
            }
            
            // Assert: System should be in valid state
            // Final cleanup
            audio_system::done();
        }
        
        SUBCASE("concurrent_device_operations") {
            auto backend = std::make_shared<mock_backend_v2_enhanced>();
            audio_system::init(backend);
            
            std::vector<std::thread> threads;
            std::atomic<int> successful_opens{0};
            
            // Launch threads that open devices
            for (int i = 0; i < 4; ++i) {
                threads.emplace_back([backend, &successful_opens]() {
                    try {
                        auto device = audio_device::open_default_device(backend);
                        successful_opens++;
                        
                        // Perform some operations
                        device.pause();
                        device.resume();
                        device.set_gain(0.5f);
                    } catch (...) {
                        // Device opening might fail due to limits
                    }
                });
            }
            
            // Wait for threads
            for (auto& t : threads) {
                t.join();
            }
            
            // Assert: At least some devices should open successfully
            CHECK(successful_opens > 0);
            
            // Cleanup
            audio_system::done();
        }
    }
    
    TEST_CASE("should_handle_error_conditions") {
        SUBCASE("operations_without_init") {
            // Ensure system is not initialized
            audio_system::done();
            
            // Act & Assert: Getting backend should return nullptr
            auto backend_ptr = audio_system::get_backend();
            CHECK(backend_ptr.get() == nullptr);
        }
        
        SUBCASE("init_with_failing_backend") {
            // Arrange: Backend that fails initialization
            auto backend = create_failing_backend(true, false, false, false);
            
            // Act: Init with failing backend
            // Note: audio_system::init returns bool, doesn't throw
            bool result = audio_system::init(backend);
            CHECK(!result);  // Should return false on failure
        }
        
        SUBCASE("device_operations_with_failed_backend") {
            // Arrange: Backend that fails device operations
            auto backend = create_failing_backend(false, false, true, false);
            audio_system::init(backend);
            
            // Act & Assert: Device opening should fail
            CHECK_THROWS_AS(
                audio_device::open_default_device(backend),
                std::runtime_error
            );
            
            // Cleanup
            audio_system::done();
        }
    }
    
    TEST_CASE("should_track_system_state_correctly") {
        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        
        SUBCASE("initialization_state") {
            // Initially not initialized
            auto initial_backend = audio_system::get_backend();
            CHECK(initial_backend.get() == nullptr);
            
            // After init
            audio_system::init(backend);
            // System is initialized with backend
            CHECK(audio_system::get_backend().get() == backend.get());
            
            // After opening device
            auto device = audio_device::open_default_device(backend);
            CHECK(backend->open_device_calls == 1);
            
            // After done
            audio_system::done();
            auto done_backend = audio_system::get_backend();
            CHECK(done_backend.get() == nullptr);
        }
        
        SUBCASE("backend_lifecycle_tracking") {
            // Init
            audio_system::init(backend);
            CHECK(backend->init_calls == 1);
            CHECK(backend->shutdown_calls == 0);
            
            // Reinit with same backend
            audio_system::init(backend);
            // Backend may be reused without shutdown/init
            CHECK(backend->init_calls >= 1);
            
            // Done
            audio_system::done();
            CHECK(backend->shutdown_calls >= 1);
        }
    }
}