// Test export configuration for internal testing
#pragma once

#ifdef MUSAC_BUILD_SHARED
    #ifdef MUSAC_INTERNAL_TESTS
        // When building tests, export internal symbols
        #ifdef _WIN32
            #define MUSAC_TEST_API __declspec(dllexport)
        #else
            #define MUSAC_TEST_API __attribute__((visibility("default")))
        #endif
    #else
        // Normal build - hide internals
        #define MUSAC_TEST_API
    #endif
#else
    // Static build - no special handling needed
    #define MUSAC_TEST_API
#endif

// Macro to register tests in shared library builds
#ifdef MUSAC_BUILD_SHARED
    #define MUSAC_TEST_REGISTRATION(name) \
        extern "C" MUSAC_TEST_API void register_##name##_tests() { \
            /* Force test registration by referencing doctest internals */ \
        }
#else
    #define MUSAC_TEST_REGISTRATION(name) /* nothing for static builds */
#endif