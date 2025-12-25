cmake_minimum_required(VERSION 3.20)

project(musac
    VERSION 0.1.0
    DESCRIPTION "MUSAC - Multi-platform Universal Sound and Audio Codecs library"
    LANGUAGES CXX C
)

# Enable Objective-C languages on Apple platforms for SDL3
if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

# ============================================================================
# Neutrino CMake Integration
# ============================================================================

include(FetchContent)

# Fetch neutrino-cmake if not already available
if(NOT DEFINED NEUTRINO_CMAKE_DIR)
    FetchContent_Declare(neutrino_cmake
        GIT_REPOSITORY https://github.com/devbrain/neutrino-cmake.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(neutrino_cmake)
    set(NEUTRINO_CMAKE_DIR "${neutrino_cmake_SOURCE_DIR}/cmake")
    list(APPEND CMAKE_MODULE_PATH "${NEUTRINO_CMAKE_DIR}")
endif()

include(NeutrinoInit)

# ============================================================================
# Build Options (using standardized naming)
# ============================================================================

# Standard options (tests, examples, install, etc.)
neutrino_define_library_options(musac)

# Audio backend options
option(NEUTRINO_MUSAC_BUILD_SDL3_BACKEND "Build SDL3 backend" ON)
option(NEUTRINO_MUSAC_BUILD_SDL2_BACKEND "Build SDL2 backend" ON)

# Language bindings options
option(NEUTRINO_MUSAC_BUILD_BINDINGS "Build language bindings" OFF)
option(NEUTRINO_MUSAC_BUILD_JAVA_BINDING "Build Java/JNI binding" ON)
option(NEUTRINO_MUSAC_BUILD_PYTHON_BINDING "Build Python binding" OFF)

# Include standard install directories
include(GNUInstallDirs)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# ============================================================================
# Compiler Configuration
# ============================================================================

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC-specific settings
if(MSVC)
    # Suppress deprecation warnings for standard C functions
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# Set visibility settings for shared libraries
if(NEUTRINO_MUSAC_BUILD_SHARED)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
    include(GenerateExportHeader)
endif()

# ============================================================================
# Dependencies
# ============================================================================

# Fetch failsafe
include(${NEUTRINO_CMAKE_DIR}/deps/failsafe.cmake)
neutrino_fetch_failsafe()

# Create thirdparty::failsafe alias for compatibility with existing code
if(TARGET neutrino::failsafe AND NOT TARGET thirdparty::failsafe)
    add_library(thirdparty::failsafe ALIAS failsafe)
endif()

# Fetch doctest for testing
if(NEUTRINO_MUSAC_BUILD_TESTS)
    include(${NEUTRINO_CMAKE_DIR}/deps/doctest.cmake)
    neutrino_fetch_doctest()
endif()

# ============================================================================
# Output Directories
# ============================================================================

set(BINARY_OUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(LIB_OUT_DIR ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_OUT_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${LIB_OUT_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${LIB_OUT_DIR})

# ============================================================================
# Project Root for Subdirectories
# ============================================================================

set(MUSAC_PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Legacy compatibility variables (for existing code)
set(MUSAC_BUILD_SHARED ${NEUTRINO_MUSAC_BUILD_SHARED})
set(MUSAC_BUILD_TESTS ${NEUTRINO_MUSAC_BUILD_TESTS})
set(MUSAC_BUILD_EXAMPLES ${NEUTRINO_MUSAC_BUILD_EXAMPLES})
set(MUSAC_BUILD_SDL3_BACKEND ${NEUTRINO_MUSAC_BUILD_SDL3_BACKEND})
set(MUSAC_BUILD_SDL2_BACKEND ${NEUTRINO_MUSAC_BUILD_SDL2_BACKEND})
set(MUSAC_BUILD_BINDINGS ${NEUTRINO_MUSAC_BUILD_BINDINGS})
set(MUSAC_BUILD_JAVA_BINDING ${NEUTRINO_MUSAC_BUILD_JAVA_BINDING})
set(MUSAC_BUILD_PYTHON_BINDING ${NEUTRINO_MUSAC_BUILD_PYTHON_BINDING})

# Determine if we're a subdirectory
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(MUSAC_IS_SUBDIRECTORY OFF)
else()
    set(MUSAC_IS_SUBDIRECTORY ON)
endif()

# FPIC setting for static builds that need PIC
set(MUSAC_BUILD_FPIC OFF)
if(NOT NEUTRINO_MUSAC_BUILD_SHARED)
    # Check if we need PIC (e.g., for embedding in shared libraries)
    if(CMAKE_POSITION_INDEPENDENT_CODE)
        set(MUSAC_BUILD_FPIC ON)
    endif()
endif()

# Warning flags from neutrino-cmake
if(NEUTRINO_MUSAC_DEVELOPER_MODE)
    set(MUSAC_WARNING_FLAGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(MUSAC_WARNING_FLAGS
            -Wall -Wextra -Wpedantic
            -Wcast-align -Wcast-qual -Wconversion
            -Wdouble-promotion -Wformat=2 -Wnon-virtual-dtor
            -Wold-style-cast -Woverloaded-virtual -Wshadow
            -Wsign-conversion -Wundef -Wunused
            -Wzero-as-null-pointer-constant
        )
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            list(APPEND MUSAC_WARNING_FLAGS -Wlogical-op -Wuseless-cast)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(MUSAC_WARNING_FLAGS /W4 /permissive- /Zc:__cplusplus)
    endif()
endif()

# ============================================================================
# Subdirectories
# ============================================================================

# Audio backends (built before main library as it sets up SDL)
if(NEUTRINO_MUSAC_BUILD_SDL3_BACKEND)
    add_subdirectory(backends/src/sdl3)
endif()

if(NEUTRINO_MUSAC_BUILD_SDL2_BACKEND)
    add_subdirectory(backends/src/sdl2)
endif()

# Main library components
add_subdirectory(src/musac)

# Examples and test data
if(NEUTRINO_MUSAC_BUILD_EXAMPLES)
    add_subdirectory(test_data)
    add_subdirectory(example)
endif()

# Unit tests
if(NEUTRINO_MUSAC_BUILD_TESTS)
    add_subdirectory(unittest)
endif()

# Language bindings
if(NEUTRINO_MUSAC_BUILD_BINDINGS)
    add_subdirectory(bindings)
endif()

# ============================================================================
# Build Configuration Summary
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "  MUSAC Build Configuration Summary")
message(STATUS "========================================")
message(STATUS "")

# Library type
if(NEUTRINO_MUSAC_BUILD_SHARED)
    message(STATUS "Library Type:        SHARED")
else()
    message(STATUS "Library Type:        STATIC")
    if(MUSAC_BUILD_FPIC)
        message(STATUS "  Position Independent Code: ON")
    endif()
endif()

message(STATUS "Build Type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "")

# Audio backends
message(STATUS "Audio Backends:")
if(NEUTRINO_MUSAC_BUILD_SDL3_BACKEND)
    message(STATUS "  SDL3 Backend:      ON")
else()
    message(STATUS "  SDL3 Backend:      OFF")
endif()

if(NEUTRINO_MUSAC_BUILD_SDL2_BACKEND)
    message(STATUS "  SDL2 Backend:      ON")
else()
    message(STATUS "  SDL2 Backend:      OFF")
endif()

message(STATUS "")

# Build options
message(STATUS "Build Options:")
message(STATUS "  Unit Tests:        ${NEUTRINO_MUSAC_BUILD_TESTS}")
message(STATUS "  Examples:          ${NEUTRINO_MUSAC_BUILD_EXAMPLES}")
message(STATUS "  Developer Mode:    ${NEUTRINO_MUSAC_DEVELOPER_MODE}")
message(STATUS "")

# Language bindings
if(NEUTRINO_MUSAC_BUILD_BINDINGS)
    message(STATUS "Language Bindings:")
    message(STATUS "  Java/JNI:          ${NEUTRINO_MUSAC_BUILD_JAVA_BINDING}")
    message(STATUS "  Python:            ${NEUTRINO_MUSAC_BUILD_PYTHON_BINDING}")
    message(STATUS "")
endif()

# Installation paths
if(NEUTRINO_MUSAC_INSTALL)
    message(STATUS "Installation:")
    message(STATUS "  Install Prefix:    ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "")
endif()

# Compiler information
message(STATUS "Compiler:")
message(STATUS "  C++ Compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "")

message(STATUS "========================================")
message(STATUS "  Configuration complete!")
message(STATUS "========================================")
message(STATUS "")

# ============================================================================
# Installation and Packaging
# ============================================================================

if(NEUTRINO_MUSAC_INSTALL AND NOT MUSAC_IS_SUBDIRECTORY)
    include(CMakePackageConfigHelpers)

    # Generate the config file from template
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/musacConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/musacConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/musac"
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR
    )

    # Generate the version file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/musacConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Install the config and version files
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/musacConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/musacConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/musac"
    )
endif()
