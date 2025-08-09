# SDL3 Backend Library
cmake_minimum_required(VERSION 3.20)

include(FetchContent)

# Inherit build type from parent musac library
if(MUSAC_BUILD_SHARED)
    set(BACKEND_LIB_TYPE SHARED)
else()
    set(BACKEND_LIB_TYPE STATIC)
endif()

# Handle SDL3 dependency
find_package(SDL3 QUIET)

if(NOT SDL3_FOUND)
    message(STATUS "SDL3 not found locally, fetching from GitHub...")
    
    set(SDL_VERSION "3.2.16")
    
    # Configure SDL3 build options
    # When building musac_backend_sdl3 as shared library, we need SDL3 with PIC
    if(MUSAC_BUILD_SHARED)
        # Build SDL3 as static with PIC for linking into our shared library
        set(SDL_STATIC ON CACHE INTERNAL "")
        set(SDL_SHARED OFF CACHE INTERNAL "")
        set(SDL_STATIC_PIC ON CACHE INTERNAL "")  # Critical: Enable PIC for static SDL3
    else()
        # Build SDL3 as static library
        set(SDL_STATIC ON CACHE INTERNAL "")
        set(SDL_SHARED OFF CACHE INTERNAL "")
        # Enable PIC if musac is being built with PIC
        if(MUSAC_BUILD_FPIC)
            set(SDL_STATIC_PIC ON CACHE INTERNAL "")
        else()
            set(SDL_STATIC_PIC OFF CACHE INTERNAL "")
        endif()
    endif()
    
    set(SDL_LIBURING OFF CACHE INTERNAL "")
    # SDL_INSTALL must be ON so SDL3 can be found by downstream users
    set(SDL_INSTALL ON CACHE INTERNAL "")
    # Disable SDL tests to prevent CTest targets from being created
    set(SDL_TESTS OFF CACHE INTERNAL "")
    set(SDL_EXAMPLES OFF CACHE INTERNAL "")
    
    FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL3-${SDL_VERSION}.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        TIMEOUT 10
        UPDATE_COMMAND ""
        OVERRIDE_FIND_PACKAGE
    )
    
    FetchContent_MakeAvailable(SDL3)
    
    # Ensure SDL3 static library has PIC when needed
    if(TARGET SDL3-static AND MUSAC_BUILD_SHARED)
        set_target_properties(SDL3-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
        message(STATUS "Enabled PIC for SDL3-static library")
    endif()
else()
    message(STATUS "Found SDL3 locally: ${SDL3_VERSION}")
endif()

# SDL3 backend sources
set(SDL3_BACKEND_SOURCES
    # Audio stream implementation
    sdl3_audio_stream.cc
    sdl3_audio_stream.hh
    # v2 backend implementation
    sdl3_backend.cc
    sdl3_backend.hh
    sdl3_backend_factory.cc
)

# Create the backend library
add_library(musac_backend_sdl3 ${BACKEND_LIB_TYPE} ${SDL3_BACKEND_SOURCES})

# Set properties
# When building as shared library, always need PIC
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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../..>
        $<BUILD_INTERFACE:${MUSAC_PROJECT_ROOT}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src/musac>  # For generated export headers
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Dependencies
target_link_libraries(musac_backend_sdl3
    PUBLIC
        musac_sdk
        SDL3::SDL3
    PRIVATE
        thirdparty::failsafe
)

# Export macro for shared library
if(MUSAC_BUILD_SHARED)
    target_compile_definitions(musac_backend_sdl3
        PRIVATE MUSAC_BACKEND_SDL3_EXPORTS
        PUBLIC MUSAC_BACKEND_SDL3_SHARED
    )
    
    # Generate export header
    include(GenerateExportHeader)
    generate_export_header(musac_backend_sdl3
        EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/export_musac_backend_sdl3.h
        EXPORT_MACRO_NAME MUSAC_BACKEND_SDL3_EXPORT
    )
    
    # Add generated header to include paths
    target_include_directories(musac_backend_sdl3
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
endif()

# Compile options (inherit from parent)
target_compile_options(musac_backend_sdl3 PRIVATE ${MUSAC_WARNING_FLAGS})

# Installation
install(TARGETS musac_backend_sdl3
    EXPORT musac-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers if needed
install(FILES 
    ${CMAKE_SOURCE_DIR}/include/musac/backends/sdl3/sdl3_backend.hh
    DESTINATION include/musac/backends/sdl3
)