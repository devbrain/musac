cmake_minimum_required(VERSION 3.14)

# Options for different bindings
option(MUSAC_BUILD_JAVA_BINDING "Build Java/JNI binding" ON)
option(MUSAC_BUILD_PYTHON_BINDING "Build Python binding" OFF)

# Java/JNI binding
if(MUSAC_BUILD_JAVA_BINDING)
    add_subdirectory(java)
endif()

# Python binding (future)
if(MUSAC_BUILD_PYTHON_BINDING)
    # add_subdirectory(python)
    message(STATUS "Python bindings not yet implemented")
endif()