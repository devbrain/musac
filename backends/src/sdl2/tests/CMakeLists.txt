cmake_minimum_required(VERSION 3.20)

# Only build tests if requested
if (NOT MUSAC_BUILD_TESTS)
    return()
endif()

# SDL2 Backend Tests
add_executable(musac_sdl2_backend_test
    test_main.cc
    test_sdl2_backend.cc
    ../../../test_common/backend_test_helpers.cc
)

target_include_directories(musac_sdl2_backend_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../test_common
)

target_link_libraries(musac_sdl2_backend_test
    PRIVATE
        doctest::doctest
        musac_backend_sdl2
        musac_sdk
        thirdparty::failsafe
)

# Add compile options
target_compile_options(musac_sdl2_backend_test PRIVATE ${MUSAC_WARNING_FLAGS})

# Register tests with CTest
include(CTest)
add_test(NAME SDL2BackendTests COMMAND musac_sdl2_backend_test)
set_tests_properties(SDL2BackendTests PROPERTIES LABELS "SDL2Backend")