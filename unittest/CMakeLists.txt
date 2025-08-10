cmake_minimum_required(VERSION 3.20)



if (NOT MUSAC_BUILD_TESTS)
    return()
endif ()

# Try to find doctest or fetch it
find_package(doctest QUIET)

if (NOT doctest_FOUND)
    message(STATUS "doctest not found, fetching from GitHub...")

    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)

    # Allow FetchContent_Populate to be used
    if (POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif ()

    set(DOCTEST_WITH_TESTS OFF CACHE INTERNAL "")
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE INTERNAL "")
    set(DOCTEST_NO_INSTALL ON CACHE INTERNAL "")

    FetchContent_Declare(
            doctest
            GIT_REPOSITORY https://github.com/onqtam/doctest.git
            GIT_TAG v2.4.12
            GIT_PROGRESS TRUE
    )

    FetchContent_MakeAvailable(doctest)
endif ()

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
        integration/backends/test_sdl3_backend.cc
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
            musac_backend_sdl3
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
            musac_backend_sdl3
            thirdparty::failsafe
    )
endif ()

# Compile options
target_compile_options(musac_unittest PRIVATE ${MUSAC_WARNING_FLAGS})

# Enable internal testing if available
if (TARGET musac_internal)
    target_compile_definitions(musac_unittest PRIVATE MUSAC_INTERNAL_TESTING)
endif()

# Register tests with CTest
include(CTest)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(musac_unittest)

# Add custom targets for running specific test types
add_custom_target(run_tests
        COMMAND musac_unittest
        DEPENDS musac_unittest
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Running all tests..."
)

add_custom_target(run_unit_tests
        COMMAND musac_unittest -tc="*unit*"
        DEPENDS musac_unittest
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Running unit tests only..."
)

add_custom_target(run_integration_tests
        COMMAND musac_unittest -tc="*integration*,*thread_safety*,*codec*,*backend*"
        DEPENDS musac_unittest
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Running integration tests only..."
)