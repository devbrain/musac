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

    # Configure SDL2 build options (Option 4: Smart defaults)
    # If building shared musac, default to static SDL (for easier deployment)
    # If building static musac, must use static SDL
    if (MUSAC_BUILD_SHARED)
        option(MUSAC_FETCHED_SDL2_STATIC "Build fetched SDL2 as static library" ON)
    else ()
        set(MUSAC_FETCHED_SDL2_STATIC ON) # Force static for static builds
    endif ()

    # Apply the build configuration
    if (MUSAC_FETCHED_SDL2_STATIC)
        set(SDL_STATIC ON CACHE INTERNAL "")
        set(SDL_SHARED OFF CACHE INTERNAL "")
        set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
        # Enable PIC if needed for static SDL2
        if (MUSAC_BUILD_SHARED OR MUSAC_BUILD_FPIC)
            set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")
        else ()
            set(CMAKE_POSITION_INDEPENDENT_CODE OFF CACHE INTERNAL "")
        endif ()
    else ()
        # Build SDL2 as shared library
        set(SDL_STATIC OFF CACHE INTERNAL "")
        set(SDL_SHARED ON CACHE INTERNAL "")
        set(BUILD_SHARED_LIBS ON CACHE INTERNAL "")
    endif ()
    
    # Always disable SDL2main
    set(SDL2_DISABLE_SDL2MAIN ON CACHE INTERNAL "")

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

# Add tests subdirectory
if (MUSAC_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Installation targets
install(TARGETS musac_backend_sdl2
    EXPORT musacTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Backend headers are installed by SDL3 backend (shared headers)
# Only install export header if not already installed by SDL3 backend
if(MUSAC_BUILD_SHARED AND NOT MUSAC_BUILD_SDL3_BACKEND)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/export_musac_backend_sdl2.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/musac
    )
endif()