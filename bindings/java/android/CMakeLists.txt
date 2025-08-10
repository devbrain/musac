cmake_minimum_required(VERSION 3.22)
project(musac_android)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android specific settings
if(ANDROID)
    # Include Android log library
    find_library(log-lib log)
    
    # Set up paths relative to this file
    set(MUSAC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
    set(JAVA_NATIVE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../native)
    
    # Add include directories
    include_directories(
        ${MUSAC_ROOT}/include
        ${MUSAC_ROOT}/src
        ${MUSAC_ROOT}
        ${JAVA_NATIVE_DIR}
    )
    
    # Collect source files (excluding files that need external dependencies)
    file(GLOB_RECURSE MUSAC_SDK_SOURCES 
        ${MUSAC_ROOT}/src/musac/sdk/*.cc
        ${MUSAC_ROOT}/src/musac/sdk/*.cpp
    )
    
    file(GLOB_RECURSE MUSAC_CODECS_SOURCES
        ${MUSAC_ROOT}/src/musac/codecs/*.cc
        ${MUSAC_ROOT}/src/musac/codecs/*.cpp
    )
    
    # Add external dependency sources directly
    file(GLOB_RECURSE FAILSAFE_SOURCES
        ${MUSAC_ROOT}/ext/failsafe/*.cpp
    )
    
    # Exclude test files
    list(FILTER MUSAC_SDK_SOURCES EXCLUDE REGEX ".*test.*")
    list(FILTER MUSAC_CODECS_SOURCES EXCLUDE REGEX ".*test.*")
    
    file(GLOB JNI_SOURCES
        ${JAVA_NATIVE_DIR}/*.cc
        ${JAVA_NATIVE_DIR}/*.cpp
    )
    
    # Create the shared library
    add_library(musac_java SHARED
        ${MUSAC_SDK_SOURCES}
        ${MUSAC_CODECS_SOURCES}
        ${JNI_SOURCES}
    )
    
    # Link Android libraries
    target_link_libraries(musac_java
        ${log-lib}
        android
    )
    
    # Set properties
    set_target_properties(musac_java PROPERTIES
        LINK_FLAGS "-Wl,--no-undefined"
    )
    
    # Define export macros for shared library build
    target_compile_definitions(musac_java PRIVATE
        MUSAC_SDK_EXPORT=
        MUSAC_CODECS_EXPORT=
        MUSAC_BUILD_SHARED=1
    )
    
    # Optimize for size in release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(musac_java PRIVATE -Os -fvisibility=hidden)
        set_target_properties(musac_java PROPERTIES
            LINK_FLAGS "-Wl,--no-undefined -Wl,--gc-sections -Wl,--strip-all"
        )
    endif()
    
endif()