cmake_minimum_required(VERSION 3.20)
project(musac
        VERSION 0.1.0
        LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(MUSAC_BUILD_SHARED "Build MUSAC as a shared library" ON)
option(MUSAC_BUILD_FPIC "Build MUSAC with position independent code (for static builds)" OFF)

# Build configuration options
option(MUSAC_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(MUSAC_ENABLE_LTO "Enable link-time optimization" OFF)

# Audio backend options
option(MUSAC_BUILD_SDL3_BACKEND "Build SDL3 backend as separate library" ON)
option(MUSAC_BUILD_SDL2_BACKEND "Build SDL2 backend as separate library" ON)

# Option to enable/disable unit tests
option(MUSAC_BUILD_TESTS "Build unit tests" ON)

# Language bindings options
option(MUSAC_BUILD_BINDINGS "Build language bindings" OFF)
option(MUSAC_BUILD_JAVA_BINDING "Build Java/JNI binding" ON)
option(MUSAC_BUILD_PYTHON_BINDING "Build Python binding" OFF)

# Include standard install directories
include(GNUInstallDirs)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()



# Set visibility settings for shared libraries
if(MUSAC_BUILD_SHARED)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
    include(GenerateExportHeader)
endif()

# Define warning flags for our project
set(MUSAC_WARNING_FLAGS)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(MUSAC_WARNING_FLAGS
            -Wall           # All standard warnings
            -Wextra         # Extra warnings
            -Wpedantic      # Strict ISO C++ compliance
            -Wcast-align    # Warn about potential performance problem casts
            -Wcast-qual     # Warn about casts that remove qualifiers
            -Wconversion    # Warn about type conversions that may lose data
            -Wdouble-promotion  # Warn about float to double promotion
            -Wformat=2      # Additional format string warnings
            -Wnon-virtual-dtor  # Warn about non-virtual destructors
            -Wold-style-cast    # Warn about C-style casts
            -Woverloaded-virtual # Warn about overloaded virtual functions
            -Wshadow        # Warn about variable shadowing
            -Wsign-conversion   # Warn about sign conversions
            -Wundef         # Warn about undefined identifiers in #if
            -Wunused        # Warn about unused entities
            -Wzero-as-null-pointer-constant # Warn about using 0 as nullptr
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        list(APPEND MUSAC_WARNING_FLAGS
                -Wlogical-op    # Warn about logical operations being used where bitwise were probably wanted
                -Wuseless-cast  # Warn about useless casts
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(MUSAC_WARNING_FLAGS
            /W4             # Highest warning level
            /permissive-    # Strict standard conformance
            /Zc:__cplusplus # Report correct __cplusplus value
    )
endif()

# Treat warnings as errors in CI/Release builds (optional)
# list(APPEND MUSAC_WARNING_FLAGS -Werror)

# Sanitizer flags
if(MUSAC_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(MUSAC_SANITIZER_FLAGS
                -fsanitize=address
                -fsanitize=undefined
                -fno-omit-frame-pointer
        )
        list(APPEND MUSAC_WARNING_FLAGS ${MUSAC_SANITIZER_FLAGS})

        # Link flags for sanitizers
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
    else()
        message(WARNING "Sanitizers are only supported with GCC and Clang")
    endif()
endif()

# Link-time optimization
if(MUSAC_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "Link-time optimization enabled")
    else()
        message(WARNING "Link-time optimization not supported: ${ipo_output}")
    endif()
endif()

# Output binary to predictable location.
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
# ===========================================================================
set (MUSAC_PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

if(MUSAC_BUILD_SDL3_BACKEND)
    add_subdirectory(backends/src/sdl3)
endif()

if(MUSAC_BUILD_SDL2_BACKEND)
    add_subdirectory(backends/src/sdl2)
endif()

add_subdirectory(ext)
add_subdirectory(src/musac)

add_subdirectory(test_data)
add_subdirectory(example)
add_subdirectory(unittest)
add_subdirectory(tools)

# Language bindings
if(MUSAC_BUILD_BINDINGS)
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
if(MUSAC_BUILD_SHARED)
    message(STATUS "Library Type:        SHARED")
else()
    message(STATUS "Library Type:        STATIC")
    if(MUSAC_BUILD_FPIC)
        message(STATUS "  Position Independent Code: ON")
    else()
        message(STATUS "  Position Independent Code: OFF")
    endif()
endif()

message(STATUS "Build Type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "")

# Audio backends
message(STATUS "Audio Backends:")
if(MUSAC_BUILD_SDL3_BACKEND)
    message(STATUS "  SDL3 Backend:      ON")
    # Check if SDL3 was fetched by looking for the source directory
    if(EXISTS "${CMAKE_BINARY_DIR}/_deps/sdl3-src")
        message(STATUS "    SDL3 Source:     Fetched from GitHub")
        if(DEFINED MUSAC_FETCHED_SDL3_STATIC)
            if(MUSAC_FETCHED_SDL3_STATIC)
                message(STATUS "    SDL3 Build:      Static")
            else()
                message(STATUS "    SDL3 Build:      Shared")
            endif()
        endif()
    else()
        message(STATUS "    SDL3 Source:     System")
    endif()
else()
    message(STATUS "  SDL3 Backend:      OFF")
endif()

if(MUSAC_BUILD_SDL2_BACKEND)
    message(STATUS "  SDL2 Backend:      ON")
    # Check if SDL2 was fetched by looking for the source directory
    if(EXISTS "${CMAKE_BINARY_DIR}/_deps/sdl2-src")
        message(STATUS "    SDL2 Source:     Fetched from GitHub")
        if(DEFINED MUSAC_FETCHED_SDL2_STATIC)
            if(MUSAC_FETCHED_SDL2_STATIC)
                message(STATUS "    SDL2 Build:      Static")
            else()
                message(STATUS "    SDL2 Build:      Shared")
            endif()
        endif()
    else()
        if(SDL2_FOUND)
            message(STATUS "    SDL2 Source:     System (${SDL2_VERSION})")
        else()
            message(STATUS "    SDL2 Source:     System")
        endif()
    endif()
else()
    message(STATUS "  SDL2 Backend:      OFF")
endif()

message(STATUS "")

# Build options
message(STATUS "Build Options:")
message(STATUS "  Unit Tests:        ${MUSAC_BUILD_TESTS}")
if(MUSAC_BUILD_TESTS)
    if(TARGET musac_backend_sdl3)
        message(STATUS "    Test Backend:    SDL3")
    elseif(TARGET musac_backend_sdl2)
        message(STATUS "    Test Backend:    SDL2")
    else()
        message(STATUS "    Test Backend:    None (limited testing)")
    endif()
endif()

message(STATUS "  Sanitizers:        ${MUSAC_ENABLE_SANITIZERS}")
message(STATUS "  LTO:               ${MUSAC_ENABLE_LTO}")
message(STATUS "")

# Language bindings
if(MUSAC_BUILD_BINDINGS)
    message(STATUS "Language Bindings:")
    message(STATUS "  Java/JNI:          ${MUSAC_BUILD_JAVA_BINDING}")
    message(STATUS "  Python:            ${MUSAC_BUILD_PYTHON_BINDING}")
    message(STATUS "")
endif()

# Installation paths
message(STATUS "Installation:")
message(STATUS "  Install Prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")

# Compiler information
message(STATUS "Compiler:")
message(STATUS "  C++ Compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "")

# Platform information
message(STATUS "Platform:")
message(STATUS "  System:            ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Processor:         ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Build Platform:    ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "")

# Output directories
message(STATUS "Output Directories:")
message(STATUS "  Runtime:           ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Library:           ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "  Archive:           ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
message(STATUS "")

message(STATUS "========================================")
message(STATUS "  Configuration complete!")
message(STATUS "========================================")
message(STATUS "")

