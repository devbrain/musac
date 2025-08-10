# SDL2 Backend Library
cmake_minimum_required(VERSION 3.20)

include(FetchContent)

# Inherit build type from parent musac library
if (MUSAC_BUILD_SHARED)
    set(BACKEND_LIB_TYPE SHARED)
else ()
    set(BACKEND_LIB_TYPE STATIC)
endif ()

# Handle SDL2 dependency
find_package(SDL2 QUIET)

if (NOT SDL2_FOUND)
    message(STATUS "SDL2 not found locally, fetching from GitHub...")

    set(SDL_VERSION "2.30.10")

    # Configure SDL2 build options
    # When building musac_backend_sdl2 as shared library, we need SDL2 with PIC
    if (MUSAC_BUILD_SHARED)
        # Build SDL2 as static with PIC for linking into our shared library
        set(SDL_STATIC ON CACHE INTERNAL "")
        set(SDL_SHARED OFF CACHE INTERNAL "")
        set(SDL2_DISABLE_SDL2MAIN ON CACHE INTERNAL "")
        set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
        set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")
    else ()
        # Build SDL2 as static library
        set(SDL_STATIC ON CACHE INTERNAL "")
        set(SDL_SHARED OFF CACHE INTERNAL "")
        set(SDL2_DISABLE_SDL2MAIN ON CACHE INTERNAL "")
        # Enable PIC if musac is being built with PIC
        if (MUSAC_BUILD_FPIC)
            set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")
        else ()
            set(CMAKE_POSITION_INDEPENDENT_CODE OFF CACHE INTERNAL "")
        endif ()
    endif ()

    # Disable SDL2 features we don't need
    set(SDL_TEST OFF CACHE INTERNAL "")
    set(SDL2_DISABLE_INSTALL OFF CACHE INTERNAL "")
    
    FetchContent_Declare(
            SDL2
            URL https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-${SDL_VERSION}.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            TIMEOUT 10
            UPDATE_COMMAND ""
    )

    FetchContent_MakeAvailable(SDL2)

    # Create an interface target to match find_package(SDL2) behavior
    if(NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 INTERFACE IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
            INTERFACE_LINK_LIBRARIES SDL2-static
            INTERFACE_INCLUDE_DIRECTORIES "${sdl2_SOURCE_DIR}/include"
        )
    endif()

    # Ensure SDL2 static library has PIC when needed
    if (TARGET SDL2-static AND MUSAC_BUILD_SHARED)
        set_target_properties(SDL2-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
        message(STATUS "Enabled PIC for SDL2-static library")
    endif ()
else ()
    message(STATUS "Found SDL2 locally: ${SDL2_VERSION}")
endif ()

# SDL2 backend sources
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

# Create the backend library
add_library(musac_backend_sdl2 ${BACKEND_LIB_TYPE} ${SDL2_BACKEND_SOURCES})
target_compile_definitions(musac_backend_sdl2 PRIVATE LOGGER_DEFAULT_CATEGORY=musac_backend_sdl2)

# Set properties
# When building as shared library, always need PIC
if (MUSAC_BUILD_SHARED)
    set(BACKEND_PIC ON)
else ()
    set(BACKEND_PIC ${MUSAC_BUILD_FPIC})
endif ()

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
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>  # For generated export headers
        $<INSTALL_INTERFACE:include>
)

# Dependencies
target_link_libraries(musac_backend_sdl2
        PUBLIC
        musac_sdk
        PRIVATE
        SDL2::SDL2
        thirdparty::failsafe
)

# Export macro for shared library
if (MUSAC_BUILD_SHARED)
    target_compile_definitions(musac_backend_sdl2
            PRIVATE MUSAC_BACKEND_SDL2_EXPORTS
            PUBLIC MUSAC_BACKEND_SDL2_SHARED
    )

    # Generate export header
    include(GenerateExportHeader)
    generate_export_header(musac_backend_sdl2
            EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/export_musac_backend_sdl2.h
            EXPORT_MACRO_NAME MUSAC_BACKEND_SDL2_EXPORT
    )

    # Add generated header to include paths
    target_include_directories(musac_backend_sdl2
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
endif ()

target_compile_options(musac_backend_sdl2 PRIVATE ${MUSAC_WARNING_FLAGS})