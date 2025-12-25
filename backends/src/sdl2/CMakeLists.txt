# SDL2 Backend Library
cmake_minimum_required(VERSION 3.20)

# ============================================================================
# SDL2 Dependency (using neutrino-cmake)
# ============================================================================

# Inherit build type from parent musac library
if(MUSAC_BUILD_SHARED)
    set(BACKEND_LIB_TYPE SHARED)
else()
    set(BACKEND_LIB_TYPE STATIC)
endif()

# Check if SDL2 target already exists (e.g., from parent project)
if(TARGET SDL2::SDL2 OR TARGET SDL2-static)
    message(STATUS "Found SDL2 target from parent project")
    set(SDL2_FOUND TRUE)
else()
    # Use neutrino-cmake to fetch SDL2
    include(${NEUTRINO_CMAKE_DIR}/deps/SDL2.cmake)

    # Configure SDL2 build options
    # If building shared musac, default to static SDL (for easier deployment)
    if(MUSAC_BUILD_SHARED)
        option(MUSAC_FETCHED_SDL2_STATIC "Build fetched SDL2 as static library" ON)
    else()
        set(MUSAC_FETCHED_SDL2_STATIC ON)
    endif()

    if(MUSAC_FETCHED_SDL2_STATIC)
        neutrino_fetch_sdl2(STATIC)
    else()
        neutrino_fetch_sdl2(SHARED)
    endif()
endif()

# ============================================================================
# SDL2 Backend Sources
# ============================================================================

set(SDL2_BACKEND_SOURCES
    # Audio stream implementation
    sdl2_audio_stream.cc
    sdl2_audio_stream.hh
    # Backend implementation
    sdl2_backend_impl.cc
    sdl2_backend_impl.hh
    sdl2_backend_factory.cc
    sdl2.hh
    ${MUSAC_PROJECT_ROOT}/backends/include/musac_backends/sdl2/sdl2_backend.hh
)

# ============================================================================
# Create Backend Library
# ============================================================================

add_library(musac_backend_sdl2 ${BACKEND_LIB_TYPE} ${SDL2_BACKEND_SOURCES})
add_library(musac::backend_sdl2 ALIAS musac_backend_sdl2)

target_compile_definitions(musac_backend_sdl2 PRIVATE LOGGER_DEFAULT_CATEGORY=musac_backend_sdl2)

# Set properties
if(MUSAC_BUILD_SHARED)
    set(BACKEND_PIC ON)
else()
    set(BACKEND_PIC ${MUSAC_BUILD_FPIC})
endif()

set_target_properties(musac_backend_sdl2 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ${BACKEND_PIC}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(musac_backend_sdl2
    PUBLIC
        $<BUILD_INTERFACE:${MUSAC_PROJECT_ROOT}/backends/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

# ============================================================================
# Dependencies
# ============================================================================

# Determine which SDL2 target to use
if(TARGET SDL2::SDL2)
    set(SDL2_TARGET SDL2::SDL2)
elseif(TARGET SDL2-static)
    set(SDL2_TARGET SDL2-static)
else()
    set(SDL2_TARGET SDL2::SDL2)
endif()

target_link_libraries(musac_backend_sdl2
    PUBLIC
        musac_sdk
    PRIVATE
        ${SDL2_TARGET}
        thirdparty::failsafe
)

# Ensure SDL2 static library has PIC when needed
if(TARGET SDL2-static AND MUSAC_BUILD_SHARED)
    set_target_properties(SDL2-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
    message(STATUS "Enabled PIC for SDL2-static library")
endif()

# ============================================================================
# Export Header
# ============================================================================

include(GenerateExportHeader)
generate_export_header(musac_backend_sdl2
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/export_musac_backend_sdl2.h
    EXPORT_MACRO_NAME MUSAC_BACKEND_SDL2_EXPORT
    STATIC_DEFINE MUSAC_BACKEND_SDL2_STATIC_DEFINE
)

target_include_directories(musac_backend_sdl2
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

if(MUSAC_BUILD_SHARED)
    target_compile_definitions(musac_backend_sdl2
        PRIVATE MUSAC_BACKEND_SDL2_EXPORTS
        PUBLIC MUSAC_BACKEND_SDL2_SHARED
    )
else()
    target_compile_definitions(musac_backend_sdl2 PUBLIC MUSAC_BACKEND_SDL2_STATIC_DEFINE)
endif()

# Warning flags
if(DEFINED MUSAC_WARNING_FLAGS)
    target_compile_options(musac_backend_sdl2 PRIVATE ${MUSAC_WARNING_FLAGS})
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
    install(TARGETS musac_backend_sdl2
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Backend headers are installed by SDL3 backend (shared headers)
    # Only install if SDL3 backend is not being built
    if(NOT MUSAC_BUILD_SDL3_BACKEND)
        install(DIRECTORY ${MUSAC_PROJECT_ROOT}/backends/include/musac_backends
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hh" PATTERN "*.h"
        )
    endif()

    # Install generated export header if shared build
    if(MUSAC_BUILD_SHARED)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/export_musac_backend_sdl2.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/musac
        )
    endif()
endif()
