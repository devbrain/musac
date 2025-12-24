# SDL3 Backend Library
cmake_minimum_required(VERSION 3.20)

# ============================================================================
# SDL3 Dependency (using neutrino-cmake)
# ============================================================================

# Inherit build type from parent musac library
if(MUSAC_BUILD_SHARED)
    set(BACKEND_LIB_TYPE SHARED)
else()
    set(BACKEND_LIB_TYPE STATIC)
endif()

# Check if SDL3 target already exists (e.g., from parent project)
if(TARGET SDL3::SDL3 OR TARGET SDL3-static)
    message(STATUS "Found SDL3 target from parent project")
    set(SDL3_FOUND TRUE)
else()
    # Use neutrino-cmake to fetch SDL3
    include(${NEUTRINO_CMAKE_DIR}/deps/SDL3.cmake)

    # Configure SDL3 build options
    # If building shared musac, default to static SDL (for easier deployment)
    if(MUSAC_BUILD_SHARED)
        option(MUSAC_FETCHED_SDL3_STATIC "Build fetched SDL3 as static library" ON)
    else()
        set(MUSAC_FETCHED_SDL3_STATIC ON)
    endif()

    if(MUSAC_FETCHED_SDL3_STATIC)
        neutrino_fetch_sdl3(STATIC)
    else()
        neutrino_fetch_sdl3(SHARED)
    endif()
endif()

# ============================================================================
# SDL3 Backend Sources
# ============================================================================

set(SDL3_BACKEND_SOURCES
    # Audio stream implementation
    sdl3_audio_stream.cc
    sdl3_audio_stream.hh
    # Backend implementation
    sdl3_backend_impl.cc
    sdl3_backend_impl.hh
    sdl3_backend_factory.cc
    sdl3.hh
    ${MUSAC_PROJECT_ROOT}/backends/include/musac_backends/sdl3/sdl3_backend.hh
)

# ============================================================================
# Create Backend Library
# ============================================================================

add_library(musac_backend_sdl3 ${BACKEND_LIB_TYPE} ${SDL3_BACKEND_SOURCES})
add_library(musac::backend_sdl3 ALIAS musac_backend_sdl3)

target_compile_definitions(musac_backend_sdl3 PRIVATE LOGGER_DEFAULT_CATEGORY=musac_backend_sdl3)

# Set properties
if(MUSAC_BUILD_SHARED)
    set(BACKEND_PIC ON)
else()
    set(BACKEND_PIC ${MUSAC_BUILD_FPIC})
endif()

set_target_properties(musac_backend_sdl3 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ${BACKEND_PIC}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(musac_backend_sdl3
    PUBLIC
        $<BUILD_INTERFACE:${MUSAC_PROJECT_ROOT}/backends/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

# ============================================================================
# Dependencies
# ============================================================================

# Determine which SDL3 target to use
if(TARGET SDL3::SDL3)
    set(SDL3_TARGET SDL3::SDL3)
elseif(TARGET SDL3-static)
    set(SDL3_TARGET SDL3-static)
else()
    set(SDL3_TARGET SDL3::SDL3)
endif()

target_link_libraries(musac_backend_sdl3
    PUBLIC
        musac_sdk
    PRIVATE
        ${SDL3_TARGET}
        thirdparty::failsafe
)

# Ensure SDL3 static library has PIC when needed
if(TARGET SDL3-static AND MUSAC_BUILD_SHARED)
    set_target_properties(SDL3-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
    message(STATUS "Enabled PIC for SDL3-static library")
endif()

# ============================================================================
# Export Header
# ============================================================================

include(GenerateExportHeader)
generate_export_header(musac_backend_sdl3
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/export_musac_backend_sdl3.h
    EXPORT_MACRO_NAME MUSAC_BACKEND_SDL3_EXPORT
    STATIC_DEFINE MUSAC_BACKEND_SDL3_STATIC_DEFINE
)

target_include_directories(musac_backend_sdl3
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

if(MUSAC_BUILD_SHARED)
    target_compile_definitions(musac_backend_sdl3
        PRIVATE MUSAC_BACKEND_SDL3_EXPORTS
        PUBLIC MUSAC_BACKEND_SDL3_SHARED
    )
else()
    target_compile_definitions(musac_backend_sdl3 PUBLIC MUSAC_BACKEND_SDL3_STATIC_DEFINE)
endif()

# Warning flags
if(DEFINED MUSAC_WARNING_FLAGS)
    target_compile_options(musac_backend_sdl3 PRIVATE ${MUSAC_WARNING_FLAGS})
endif()

# ============================================================================
# Tests
# ============================================================================

if(MUSAC_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ============================================================================
# Installation
# ============================================================================

if(NOT MUSAC_IS_SUBDIRECTORY AND NEUTRINO_MUSAC_INSTALL)
    install(TARGETS musac_backend_sdl3
        EXPORT musacTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install backend headers
    install(DIRECTORY ${MUSAC_PROJECT_ROOT}/backends/include/musac_backends
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hh" PATTERN "*.h"
    )

    # Install generated export header if shared build
    if(MUSAC_BUILD_SHARED)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/export_musac_backend_sdl3.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/musac
        )
    endif()
endif()
