#include <doctest/doctest.h>
#include <musac_backends/sdl3/sdl3_backend.hh>
#include "../../../test_common/backend_test_helpers.hh"
#include <memory>

TEST_SUITE("SDL3Backend") {
    TEST_CASE("SDL3 backend creation") {
        auto backend = musac::create_sdl3_backend();
        CHECK(backend != nullptr);
        CHECK_FALSE(backend->is_initialized());
        CHECK(backend->get_name() == "SDL3");
    }
    
    TEST_CASE("SDL3 initialization lifecycle") {
        musac::test::test_backend_initialization(musac::create_sdl3_backend());
    }
    
    TEST_CASE("SDL3 device enumeration") {
        musac::test::test_device_enumeration(musac::create_sdl3_backend());
    }
    
    TEST_CASE("SDL3 device open and close") {
        musac::test::test_device_open_close(musac::create_sdl3_backend());
    }
    
    TEST_CASE("SDL3 device control") {
        musac::test::test_device_control(musac::create_sdl3_backend());
    }
    
    TEST_CASE("SDL3 backend capabilities") {
        musac::test::test_backend_capabilities(musac::create_sdl3_backend());
    }
    
    TEST_CASE("SDL3 multiple devices") {
        musac::test::test_multiple_devices(musac::create_sdl3_backend());
    }
    
    TEST_CASE("SDL3 stream creation") {
        musac::test::test_stream_creation(musac::create_sdl3_backend());
    }
    
    TEST_CASE("SDL3 error conditions") {
        musac::test::test_error_conditions(musac::create_sdl3_backend());
    }
    
    // SDL3-specific tests can be added here
    TEST_CASE("SDL3 specific features") {
        auto backend = musac::create_sdl3_backend();
        REQUIRE(backend != nullptr);
        
        // Add any SDL3-specific test cases here
        // For example, testing SDL3-only features or behaviors
    }
}