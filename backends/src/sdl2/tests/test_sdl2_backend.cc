#include <doctest/doctest.h>
#include <musac_backends/sdl2/sdl2_backend.hh>
#include "../../../test_common/backend_test_helpers.hh"
#include <memory>

TEST_SUITE("SDL2Backend") {
    TEST_CASE("SDL2 backend creation") {
        auto backend = musac::create_sdl2_backend();
        CHECK(backend != nullptr);
        CHECK_FALSE(backend->is_initialized());
        CHECK(backend->get_name() == "SDL2");
    }
    
    TEST_CASE("SDL2 initialization lifecycle") {
        musac::test::test_backend_initialization(musac::create_sdl2_backend());
    }
    
    TEST_CASE("SDL2 device enumeration") {
        musac::test::test_device_enumeration(musac::create_sdl2_backend());
    }
    
    TEST_CASE("SDL2 device open and close") {
        musac::test::test_device_open_close(musac::create_sdl2_backend());
    }
    
    TEST_CASE("SDL2 device control") {
        musac::test::test_device_control(musac::create_sdl2_backend());
    }
    
    TEST_CASE("SDL2 backend capabilities") {
        musac::test::test_backend_capabilities(musac::create_sdl2_backend());
    }
    
    TEST_CASE("SDL2 multiple devices") {
        musac::test::test_multiple_devices(musac::create_sdl2_backend());
    }
    
    TEST_CASE("SDL2 stream creation") {
        musac::test::test_stream_creation(musac::create_sdl2_backend());
    }
    
    TEST_CASE("SDL2 error conditions") {
        musac::test::test_error_conditions(musac::create_sdl2_backend());
    }
    
    // SDL2-specific tests can be added here
    TEST_CASE("SDL2 specific features") {
        auto backend = musac::create_sdl2_backend();
        REQUIRE(backend != nullptr);
        
        // Add any SDL2-specific test cases here
        // For example, testing SDL2-only features or behaviors
    }
    
    TEST_CASE("SDL2 mute functionality") {
        auto backend = musac::create_sdl2_backend();
        REQUIRE(backend != nullptr);
        
        SUBCASE("backend reports mute support") {
            CHECK(backend->supports_mute() == true);
        }
        
        SUBCASE("mute and unmute device") {
            backend->init();
            
            // Open a device
            musac::audio_spec spec;
            spec.freq = 44100;
            spec.format = musac::audio_format::s16le;
            spec.channels = 2;
            
            musac::audio_spec obtained_spec;
            uint32_t device_handle = backend->open_device("", spec, obtained_spec);
            CHECK(device_handle != 0);
            
            // Initially not muted
            CHECK(backend->is_device_muted(device_handle) == false);
            
            // Mute the device
            bool mute_result = backend->mute_device(device_handle);
            CHECK(mute_result == true);
            CHECK(backend->is_device_muted(device_handle) == true);
            
            // Device should be paused when muted (SDL2 uses pause for mute)
            CHECK(backend->is_device_paused(device_handle) == true);
            
            // Unmute the device
            bool unmute_result = backend->unmute_device(device_handle);
            CHECK(unmute_result == true);
            CHECK(backend->is_device_muted(device_handle) == false);
            
            // Device should be resumed when unmuted
            CHECK(backend->is_device_paused(device_handle) == false);
            
            // Clean up
            backend->close_device(device_handle);
        }
        
        SUBCASE("mute state persists independently") {
            backend->init();
            
            musac::audio_spec spec;
            spec.freq = 44100;
            spec.format = musac::audio_format::s16le;
            spec.channels = 2;
            
            musac::audio_spec obtained_spec;
            uint32_t device_handle = backend->open_device("", spec, obtained_spec);
            
            // Mute the device
            backend->mute_device(device_handle);
            CHECK(backend->is_device_muted(device_handle) == true);
            
            // Multiple mute calls should be idempotent
            backend->mute_device(device_handle);
            backend->mute_device(device_handle);
            CHECK(backend->is_device_muted(device_handle) == true);
            
            // Single unmute should unmute
            backend->unmute_device(device_handle);
            CHECK(backend->is_device_muted(device_handle) == false);
            
            // Multiple unmute calls should be idempotent
            backend->unmute_device(device_handle);
            backend->unmute_device(device_handle);
            CHECK(backend->is_device_muted(device_handle) == false);
            
            backend->close_device(device_handle);
        }
        
        backend->shutdown();
    }
}