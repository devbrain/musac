cmake_minimum_required(VERSION 3.20)

# Option to enable/disable unit tests
option(MUSAC_BUILD_TESTS "Build unit tests" ON)

if(NOT MUSAC_BUILD_TESTS)
    return()
endif()

# Try to find doctest or fetch it
find_package(doctest QUIET)

if(NOT doctest_FOUND)
    message(STATUS "doctest not found, fetching from GitHub...")
    
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    
    # Allow FetchContent_Populate to be used
    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()
    
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
endif()

# Create test executable
add_executable(musac_unittest
    # Main test runner
    test_main.cc
    
    # Core tests
    core/test_types.cc
    core/test_endian.cc
    core/test_io_stream.cc
    core/test_audio_format.cc
    core/test_memory.cc
    core/test_audio_stream.cc
    core/test_audio_device.cc
    core/test_thread_safety.cc
    core/test_cleanup.cc
    
    # SDK tests
    sdk/test_audio_converter.cc
    sdk/test_samples_converter.cc
    sdk/test_decoder_base.cc
    
    # Codec tests
    codecs/test_decoder_aiff.cc
    codecs/test_decoder_voc.cc
    codecs/test_decoder_wav.cc
    codecs/test_decoders_with_golden_data.cc
    codecs/test_decoders_with_golden_data_synth.cc
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
target_link_libraries(musac_unittest
    PRIVATE
        doctest::doctest
        musac
        musac_sdk
        musac_codecs
        thirdparty::failsafe
)

# Compile options
target_compile_options(musac_unittest PRIVATE ${MUSAC_WARNING_FLAGS})

# Register tests with CTest
include(CTest)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(musac_unittest)

# Add custom target for running tests
add_custom_target(run_tests
    COMMAND musac_unittest
    DEPENDS musac_unittest
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running unit tests..."
)