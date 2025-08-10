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
}