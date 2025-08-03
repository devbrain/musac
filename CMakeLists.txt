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
option(MUSAC_INSTALL "Generate install targets" ON)
# Build configuration options
option(MUSAC_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(MUSAC_ENABLE_LTO "Enable link-time optimization" OFF)

# Audio backend options
option(MUSAC_USE_SDL3 "Use SDL3 as audio backend" ON)
set(MUSAC_AUDIO_BACKEND "SDL3" CACHE STRING "Audio backend to use")
set_property(CACHE MUSAC_AUDIO_BACKEND PROPERTY STRINGS "SDL3" "Null" "Custom")

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
set (MUSAC_PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(ext)
add_subdirectory(src/musac)
add_subdirectory(example)

# Add unittest directory
add_subdirectory(unittest)

# Add benchmarks directory
option(MUSAC_BUILD_BENCHMARKS "Build benchmarks" OFF)
if(MUSAC_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Add tools directory
add_subdirectory(tools)

# Test decoder output program (temporarily disabled for SDL migration)
# add_executable(test_decoder_output test_decoder_output.cc)
# target_link_libraries(test_decoder_output musac SDL3::SDL3)
# target_include_directories(test_decoder_output PRIVATE include)

# Test single decoder program (temporarily disabled for SDL migration)
# add_executable(test_single_decoder test_single_decoder.cc)
# target_link_libraries(test_single_decoder musac SDL3::SDL3)
# target_include_directories(test_single_decoder PRIVATE include)

# Test simple open program (temporarily disabled for SDL migration)
# add_executable(test_simple_open test_simple_open.cc)
# target_link_libraries(test_simple_open musac_codecs musac_sdk SDL3::SDL3 thirdparty::failsafe)
# target_include_directories(test_simple_open PRIVATE include)
