cmake_minimum_required(VERSION 3.14)
project(musac_java)

# Find JNI
find_package(JNI REQUIRED)

# Source files
set(JAVA_NATIVE_SOURCES
    native/jni_io_stream.cc
    native/jni_audio_format.cc
    native/jni_decoder.cc
)

# Create shared library
add_library(musac_java SHARED ${JAVA_NATIVE_SOURCES})

# Include directories
target_include_directories(musac_java 
    PRIVATE 
    ${JNI_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/native
)

# Link libraries
target_link_libraries(musac_java
    PRIVATE
    musac_codecs
    musac_sdk
)

# Set properties
set_target_properties(musac_java PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Platform-specific settings
if(ANDROID)
    target_link_libraries(musac_java PRIVATE log)
elseif(APPLE)
    set_target_properties(musac_java PROPERTIES
        INSTALL_RPATH "@loader_path"
    )
elseif(UNIX)
    set_target_properties(musac_java PROPERTIES
        INSTALL_RPATH "$ORIGIN"
    )
endif()

# Install targets
install(TARGETS musac_java
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Java compilation (optional, for testing)
find_package(Java COMPONENTS Development)
if(Java_Development_FOUND)
    set(JAVA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set(JAVA_CLASS_DIR ${CMAKE_CURRENT_BINARY_DIR}/classes)
    
    file(MAKE_DIRECTORY ${JAVA_CLASS_DIR})
    
    # Find all Java source files
    file(GLOB_RECURSE JAVA_SOURCES ${JAVA_SOURCE_DIR}/**/*.java)
    
    # Compile Java sources
    add_custom_target(musac_java_classes ALL
        COMMAND ${Java_JAVAC_EXECUTABLE} 
                -d ${JAVA_CLASS_DIR}
                -sourcepath ${JAVA_SOURCE_DIR}
                ${JAVA_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling Java classes"
    )
    
    # Create JAR file
    add_custom_target(musac_java_jar ALL
        COMMAND ${Java_JAR_EXECUTABLE} cf musac.jar -C ${JAVA_CLASS_DIR} .
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS musac_java_classes
        COMMENT "Creating musac.jar"
    )
    
    message(STATUS "Java development tools found - will compile Java classes")
else()
    message(STATUS "Java development tools not found - skipping Java compilation")
endif()