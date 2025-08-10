cmake_minimum_required(VERSION 3.14)
project(musac_wasm)

# Check if Emscripten
if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "This CMakeLists.txt is for Emscripten builds only. Use emcmake/emmake.")
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
set(MUSAC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(JAVA_NATIVE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../native)

# Include directories
include_directories(
    ${MUSAC_ROOT}/include
    ${MUSAC_ROOT}/src
    ${MUSAC_ROOT}
    ${JAVA_NATIVE_DIR}
)

# Collect source files
file(GLOB_RECURSE MUSAC_SDK_SOURCES 
    ${MUSAC_ROOT}/src/musac/sdk/*.cc
    ${MUSAC_ROOT}/src/musac/sdk/*.cpp
)

file(GLOB_RECURSE MUSAC_CODECS_SOURCES
    ${MUSAC_ROOT}/src/musac/codecs/*.cc
    ${MUSAC_ROOT}/src/musac/codecs/*.cpp
)

# Exclude test files
list(FILTER MUSAC_SDK_SOURCES EXCLUDE REGEX ".*test.*")
list(FILTER MUSAC_CODECS_SOURCES EXCLUDE REGEX ".*test.*")

# Create WASM module (not using JNI, but direct JavaScript bindings)
add_executable(musac_wasm
    ${MUSAC_SDK_SOURCES}
    ${MUSAC_CODECS_SOURCES}
    src/wasm_bindings.cc
)

# Define export macros
target_compile_definitions(musac_wasm PRIVATE
    MUSAC_SDK_EXPORT=
    MUSAC_CODECS_EXPORT=
    MUSAC_BUILD_SHARED=1
)

# Emscripten settings for optimal web performance
set(EM_FLAGS
    "-Os"  # Optimize for size
    "-s WASM=1"
    "-s MODULARIZE=1"
    "-s EXPORT_NAME='MusacModule'"
    "-s ALLOW_MEMORY_GROWTH=1"
    "-s MAXIMUM_MEMORY=128MB"
    "-s INITIAL_MEMORY=16MB"
    "-s NO_EXIT_RUNTIME=1"
    "-s ENVIRONMENT=web"
    "-s EXPORT_ES6=1"
    "-s USE_ES6_IMPORT_META=0"
    "-s SINGLE_FILE=0"  # Separate .wasm file for better caching
    "-s FILESYSTEM=0"   # No filesystem needed
    "-s DISABLE_EXCEPTION_CATCHING=0"  # Enable exceptions
    "--bind"  # Use Embind for C++/JS bindings
)

# Additional optimizations for release
set(EM_FLAGS_RELEASE
    "-flto"  # Link-time optimization
    "--closure 1"  # Closure compiler optimization
    "-s ASSERTIONS=0"
)

# Debug flags
set(EM_FLAGS_DEBUG
    "-g"
    "-s ASSERTIONS=2"
    "-s SAFE_HEAP=1"
    "-s STACK_OVERFLOW_CHECK=2"
)

# Apply flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(JOIN " " EM_LINK_FLAGS ${EM_FLAGS} ${EM_FLAGS_DEBUG})
else()
    string(JOIN " " EM_LINK_FLAGS ${EM_FLAGS} ${EM_FLAGS_RELEASE})
endif()

set_target_properties(musac_wasm PROPERTIES
    LINK_FLAGS "${EM_LINK_FLAGS}"
    COMPILE_FLAGS "-Os -fno-exceptions -fno-rtti"
)

# Output files
set_target_properties(musac_wasm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build
)