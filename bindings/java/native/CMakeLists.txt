cmake_minimum_required(VERSION 3.10)
project(musac_java_native)

# Find JNI
find_package(JNI REQUIRED)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add parent musac project
set(MUSAC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# Build musac libraries first
add_subdirectory(${MUSAC_ROOT} ${CMAKE_BINARY_DIR}/musac)

# Include directories
include_directories(
    ${JNI_INCLUDE_DIRS}
    ${MUSAC_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Source files
set(JNI_SOURCES
    jni_common.h
    jni_decoder.cc
    jni_audio_format.h
    jni_audio_format.cc
    jni_io_stream.h
    jni_io_stream.cc
)

# Create JNI library
add_library(musac_java SHARED ${JNI_SOURCES})

# Link with musac libraries
target_link_libraries(musac_java
    ${JNI_LIBRARIES}
    musac
    musac_sdk
    musac_codecs
)

# Set output name (remove lib prefix on Windows)
if(WIN32)
    set_target_properties(musac_java PROPERTIES PREFIX "")
endif()

# Platform-specific settings
if(APPLE)
    set_target_properties(musac_java PROPERTIES
        LINK_FLAGS "-framework CoreAudio -framework AudioToolbox"
    )
elseif(UNIX)
    target_link_libraries(musac_java pthread dl m)
endif()

# Export library location
set(MUSAC_JAVA_LIBRARY $<TARGET_FILE:musac_java> PARENT_SCOPE)