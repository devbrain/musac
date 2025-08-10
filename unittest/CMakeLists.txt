cmake_minimum_required(VERSION 3.20)



if (NOT MUSAC_BUILD_TESTS)
    return()
endif ()

# Doctest should be available from ext/doctest
if (NOT TARGET doctest::doctest)
    message(FATAL_ERROR "doctest not available. Please ensure ext/doctest is included before unittest")
endif()

# Check that at least one backend is available
set(MUSAC_TEST_BACKEND "")
if(TARGET musac_backend_sdl3)
    set(MUSAC_TEST_BACKEND "musac_backend_sdl3")
    message(STATUS "Using SDL3 backend for core tests")
elseif(TARGET musac_backend_sdl2)
    set(MUSAC_TEST_BACKEND "musac_backend_sdl2")
    message(STATUS "Using SDL2 backend for core tests")
else()
    message(WARNING "No backend available for core tests. Tests requiring a backend will be skipped.")
endif()

# Create test executable
add_executable(musac_unittest
        # Main test runner
        test_main.cc

        # === UNIT TESTS (with mocks) ===
        # Core unit tests
        unit/core/test_audio_device_unit.cc
        unit/core/test_audio_stream_unit.cc
        unit/core/test_audio_system_unit.cc
        unit/core/test_mixer_unit.cc
        unit/core/test_error_scenarios.cc
        
        # === INTEGRATION TESTS (with real backends) ===
        # Core integration tests
        integration/core/test_types.cc
        integration/core/test_endian.cc
        integration/core/test_io_stream.cc
        integration/core/test_audio_format.cc
        integration/core/test_memory.cc
        integration/core/test_audio_stream.cc
        integration/core/test_audio_device.cc
        integration/core/test_cleanup.cc
        integration/core/test_device_cleanup.cc
        integration/core/test_mixer_public_api.cc
        integration/core/test_mixer_simple.cc
        integration/core/test_sdl_shutdown_order.cc
        integration/core/test_device_switching.cc
        integration/core/test_audio_system_api.cc
        integration/core/test_buffer_shrinking.cc
        integration/core/test_stream_move.cc
        integration/core/test_audio_device.cc
        integration/core/test_audio_system.cc
        
        # Comprehensive thread safety tests (merged)
        integration/core/test_thread_safety_comprehensive.cc
        # Individual thread safety tests (to be deprecated)
        integration/core/test_thread_safety.cc
        integration/core/test_phase1_thread_safety.cc
        integration/core/test_phase4_sdl_callback_safety.cc
        integration/core/test_phase5_integration.cc
        integration/core/test_internal_mixer_safety.cc

        # SDK integration tests
        integration/sdk/test_samples_converter.cc
        integration/sdk/test_decoder_base.cc
        integration/sdk/test_audio_converter_comprehensive.cc
        
        # SDK unit tests
        unit/sdk/test_audio_converter.cc
        
        # PC Speaker and MML tests
        unit/core/test_mml_parser.cc

        # Backend integration tests
        integration/backends/test_backend_interface.cc
        # Backend-specific tests moved to backends/src/*/tests/
        integration/backends/compile_test_backend.cc

        # Codec integration tests
        integration/codecs/test_decoder_aiff.cc
        integration/codecs/test_decoder_voc.cc
        integration/codecs/test_decoder_wav.cc
        integration/codecs/test_decoders_with_golden_data.cc
        integration/codecs/test_decoders_with_golden_data_synth.cc
)

# Include directories
target_include_directories(musac_unittest
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}  # For generated test headers
        ${CMAKE_CURRENT_BINARY_DIR}/..  # Alternative path for generated headers
)

# Link libraries
if (MUSAC_BUILD_SHARED AND TARGET musac_internal)
    # Use internal static library for testing internals
    target_link_libraries(musac_unittest
            PRIVATE
            doctest::doctest
            musac_internal  # Static library with internal symbols
            musac_sdk
            musac_codecs
            thirdparty::failsafe
    )
else ()
    # Use regular library (static build or no internal testing)
    target_link_libraries(musac_unittest
            PRIVATE
            doctest::doctest
            musac
            musac_sdk
            musac_codecs
            thirdparty::failsafe
    )
endif ()

# Link with available backend (if any)
if(MUSAC_TEST_BACKEND)
    target_link_libraries(musac_unittest PRIVATE ${MUSAC_TEST_BACKEND})
    target_compile_definitions(musac_unittest PRIVATE HAS_BACKEND_AVAILABLE)
endif()

# Compile options
target_compile_options(musac_unittest PRIVATE ${MUSAC_WARNING_FLAGS})

# Enable internal testing if available
if (TARGET musac_internal)
    target_compile_definitions(musac_unittest PRIVATE MUSAC_INTERNAL_TESTING)
endif()

