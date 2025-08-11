#ifndef MUSAC_UNITTEST_BACKEND_SELECTION_HH
#define MUSAC_UNITTEST_BACKEND_SELECTION_HH

#include <musac/sdk/audio_backend.hh>
#include <memory>
#include <doctest/doctest.h>

// Conditional backend includes
#ifdef MUSAC_HAS_SDL3_BACKEND
#include <musac_backends/sdl3/sdl3_backend.hh>
#elif MUSAC_HAS_SDL2_BACKEND
#include <musac_backends/sdl2/sdl2_backend.hh>
#endif

namespace musac::test {

    /**
     * Creates an appropriate audio backend based on compile-time configuration.
     * This centralizes backend selection logic for all unit tests.
     * 
     * @return A shared pointer to the created backend, or nullptr if no backend is available
     */
    inline std::shared_ptr<audio_backend> create_backend() {
#ifdef MUSAC_HAS_SDL3_BACKEND
        return std::shared_ptr<audio_backend>(create_sdl3_backend());
#elif MUSAC_HAS_SDL2_BACKEND
        return std::shared_ptr<audio_backend>(create_sdl2_backend());
#else
        return nullptr;
#endif
    }

    /**
     * Checks if a backend is available.
     * 
     * @return true if a backend can be created, false otherwise
     */
    inline bool has_backend_available() {
#if defined(MUSAC_HAS_SDL3_BACKEND) || defined(MUSAC_HAS_SDL2_BACKEND)
        return true;
#else
        return false;
#endif
    }

    /**
     * Helper macro to check if a backend is available.
     * When no backend is available, the test will simply not create one.
     * Tests should check if backend is nullptr before using it.
     */
#define REQUIRE_BACKEND() \
    do { \
        if (!musac::test::has_backend_available()) { \
            return; \
        } \
    } while(0)

    /**
     * Helper to create and initialize a backend.
     * Returns nullptr if no backend is available.
     * 
     * @return An initialized backend or nullptr
     */
    inline std::shared_ptr<audio_backend> create_initialized_backend() {
        auto backend = create_backend();
        if (backend) {
            backend->init();
        }
        return backend;
    }

} // namespace musac::test

#endif // MUSAC_UNITTEST_BACKEND_SELECTION_HH