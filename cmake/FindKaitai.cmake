# FindKaitai.cmake - Find Kaitai Struct compiler and runtime
#
# This module defines:
#  KAITAI_COMPILER - Path to kaitai-struct-compiler executable
#  KAITAI_FOUND - True if compiler was found
#  Kaitai::Runtime - Imported target for C++ runtime library
#
# Functions:
#  kaitai_generate_cpp(OUTPUT_VAR KSY_FILE [NAMESPACE ns] [OUTPUT_DIR dir])

include(FetchContent)

# Don't look for system installation - always use local version for portability
set(KAITAI_COMPILER "")

# Download portable universal release (works on all platforms via JVM)
if(NOT KAITAI_COMPILER OR NOT EXISTS ${KAITAI_COMPILER})
    message(STATUS "Downloading portable Kaitai Struct compiler...")
    
    # Download the universal release that includes everything
    set(KAITAI_VERSION "0.10")
    set(KAITAI_COMPILER_URL "https://github.com/kaitai-io/kaitai_struct_compiler/releases/download/${KAITAI_VERSION}/kaitai-struct-compiler-${KAITAI_VERSION}.zip")
    
    FetchContent_Declare(
        kaitai_compiler_download
        URL ${KAITAI_COMPILER_URL}
        SOURCE_DIR ${CMAKE_BINARY_DIR}/kaitai-compiler
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    
    FetchContent_MakeAvailable(kaitai_compiler_download)
    
    # The universal release includes a script that works on all platforms
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(KAITAI_COMPILER_SCRIPT "${CMAKE_BINARY_DIR}/kaitai-compiler/bin/kaitai-struct-compiler.bat")
    else()
        set(KAITAI_COMPILER_SCRIPT "${CMAKE_BINARY_DIR}/kaitai-compiler/bin/kaitai-struct-compiler")
        # Make sure it's executable on Unix-like systems
        execute_process(COMMAND chmod +x ${KAITAI_COMPILER_SCRIPT})
    endif()
    
    if(EXISTS ${KAITAI_COMPILER_SCRIPT})
        set(KAITAI_COMPILER ${KAITAI_COMPILER_SCRIPT})
    else()
        message(WARNING "Failed to find Kaitai compiler script after download")
    endif()
endif()

if(KAITAI_COMPILER)
    message(STATUS "Found Kaitai compiler: ${KAITAI_COMPILER}")
    set(KAITAI_FOUND TRUE)
else()
    message(WARNING "Kaitai compiler not found")
    set(KAITAI_FOUND FALSE)
endif()

# Fetch C++ runtime library
FetchContent_Declare(
    kaitai_cpp_runtime
    GIT_REPOSITORY https://github.com/kaitai-io/kaitai_struct_cpp_stl_runtime.git
    GIT_TAG        0.10.1
    GIT_SHALLOW    TRUE
)

# Disable building tests for Kaitai runtime
set(BUILD_TESTS OFF CACHE BOOL "Disable Kaitai tests" FORCE)

FetchContent_GetProperties(kaitai_cpp_runtime)
if(NOT kaitai_cpp_runtime_POPULATED)
    FetchContent_MakeAvailable(kaitai_cpp_runtime)
    
    # Create runtime library target
    add_library(kaitai_runtime STATIC
        ${kaitai_cpp_runtime_SOURCE_DIR}/kaitai/kaitaistream.cpp
    )
    
    target_include_directories(kaitai_runtime PUBLIC
        ${kaitai_cpp_runtime_SOURCE_DIR}
    )
    
    # Set C++11 requirement
    target_compile_features(kaitai_runtime PUBLIC cxx_std_11)
    
    # Enable position independent code for shared library builds
    set_property(TARGET kaitai_runtime PROPERTY POSITION_INDEPENDENT_CODE ON)
    
    # Configure string encoding - use iconv on Unix, Win32 API on Windows
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_compile_definitions(kaitai_runtime PUBLIC KS_STR_ENCODING_WIN32API)
    else()
        target_compile_definitions(kaitai_runtime PUBLIC KS_STR_ENCODING_ICONV)
    endif()
    
    # Create alias for consistent naming
    add_library(Kaitai::Runtime ALIAS kaitai_runtime)
endif()

# Function to generate C++ from .ksy files
function(kaitai_generate_cpp OUTPUT_VAR KSY_FILE)
    cmake_parse_arguments(ARG "" "NAMESPACE;OUTPUT_DIR" "" ${ARGN})
    
    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/kaitai_generated)
    endif()
    
    if(NOT ARG_NAMESPACE)
        set(ARG_NAMESPACE "musac_kaitai")
    endif()
    
    get_filename_component(KSY_NAME ${KSY_FILE} NAME_WE)
    set(GEN_CPP ${ARG_OUTPUT_DIR}/${KSY_NAME}.cpp)
    set(GEN_H ${ARG_OUTPUT_DIR}/${KSY_NAME}.h)
    
    # Create output directory
    file(MAKE_DIRECTORY ${ARG_OUTPUT_DIR})
    
    # Add custom command to generate C++ code
    add_custom_command(
        OUTPUT ${GEN_CPP} ${GEN_H}
        COMMAND ${KAITAI_COMPILER}
                --target cpp_stl
                --cpp-namespace ${ARG_NAMESPACE}
                --outdir ${ARG_OUTPUT_DIR}
                ${KSY_FILE}
        DEPENDS ${KSY_FILE}
        COMMENT "Generating C++ parser for ${KSY_NAME} from Kaitai specification"
        VERBATIM
    )
    
    # Return the generated files
    set(${OUTPUT_VAR} ${GEN_CPP} ${GEN_H} PARENT_SCOPE)
endfunction()