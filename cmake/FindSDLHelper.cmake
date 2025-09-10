# FindSDLHelper.cmake
# 
# Provides a helper function to find and link SDL2 or SDL3 libraries
# with fallback to pkg-config if CMake find_package fails.
#
# Usage:
#   find_sdl_library(
#     VERSION <2|3>              # SDL version to find (required)
#     TARGET <target_name>       # Target to link SDL to (required)
#     REQUIRED                   # Make SDL a required dependency (optional)
#     QUIET                      # Suppress status messages (optional)
#   )
#
# Example:
#   find_sdl_library(VERSION 3 TARGET imgui_lib)
#   find_sdl_library(VERSION 2 TARGET my_app REQUIRED)
#
# The function will:
# 1. Try find_package(SDL2/SDL3)
# 2. If that fails, try pkg-config
# 3. Link the appropriate SDL library to the target
# 4. Set up include directories if needed

include(CMakeParseArguments)

function(find_sdl_library)
    # Parse arguments
    cmake_parse_arguments(
        FIND_SDL
        "REQUIRED;QUIET"
        "VERSION;TARGET"
        ""
        ${ARGN}
    )
    
    # Validate required arguments
    if(NOT FIND_SDL_VERSION)
        message(FATAL_ERROR "find_sdl_library: VERSION argument is required")
    endif()
    
    if(NOT FIND_SDL_TARGET)
        message(FATAL_ERROR "find_sdl_library: TARGET argument is required")
    endif()
    
    # Validate version
    if(NOT FIND_SDL_VERSION MATCHES "^[23]$")
        message(FATAL_ERROR "find_sdl_library: VERSION must be 2 or 3")
    endif()
    
    # Set up package name and variables
    set(SDL_NAME "SDL${FIND_SDL_VERSION}")
    set(SDL_TARGET "${SDL_NAME}::${SDL_NAME}")
    set(SDL_FOUND FALSE)
    
    # Set quiet flag for find commands
    if(FIND_SDL_QUIET)
        set(QUIET_FLAG QUIET)
    else()
        set(QUIET_FLAG "")
    endif()
    
    # Try find_package first
    find_package(${SDL_NAME} ${QUIET_FLAG})
    
    if(${SDL_NAME}_FOUND)
        set(SDL_FOUND TRUE)
        if(NOT FIND_SDL_QUIET)
            message(STATUS "Found ${SDL_NAME} via find_package")
        endif()
        
        # Link using the standard CMake target
        target_link_libraries(${FIND_SDL_TARGET} PUBLIC ${SDL_TARGET})
        
    else()
        # Try pkg-config as fallback
        find_package(PkgConfig ${QUIET_FLAG})
        
        if(PKG_CONFIG_FOUND)
            # SDL3 uses "sdl3" in pkg-config, SDL2 uses "sdl2"
            string(TOLOWER "${SDL_NAME}" SDL_PKG_NAME)
            pkg_check_modules(${SDL_NAME}_PC ${QUIET_FLAG} ${SDL_PKG_NAME})
            
            if(${SDL_NAME}_PC_FOUND)
                set(SDL_FOUND TRUE)
                if(NOT FIND_SDL_QUIET)
                    message(STATUS "Found ${SDL_NAME} via pkg-config")
                endif()
                
                # Add include directories and link libraries from pkg-config
                target_include_directories(${FIND_SDL_TARGET} PUBLIC ${${SDL_NAME}_PC_INCLUDE_DIRS})
                target_link_libraries(${FIND_SDL_TARGET} PUBLIC ${${SDL_NAME}_PC_LIBRARIES})
                
                # Add any additional compile flags if needed
                if(${SDL_NAME}_PC_CFLAGS_OTHER)
                    target_compile_options(${FIND_SDL_TARGET} PUBLIC ${${SDL_NAME}_PC_CFLAGS_OTHER})
                endif()
            endif()
        endif()
    endif()
    
    # Handle REQUIRED flag
    if(FIND_SDL_REQUIRED AND NOT SDL_FOUND)
        message(FATAL_ERROR "Could not find ${SDL_NAME} (tried find_package and pkg-config)")
    endif()
    
    # Set output variable in parent scope
    set(${SDL_NAME}_FOUND ${SDL_FOUND} PARENT_SCOPE)
    
    # Return status
    if(NOT SDL_FOUND AND NOT FIND_SDL_QUIET)
        message(WARNING "${SDL_NAME} not found for target ${FIND_SDL_TARGET}")
    endif()
endfunction()

# Helper macro for backwards compatibility - find SDL without linking
# This just wraps find_package and pkg-config detection
macro(find_sdl VERSION)
    set(SDL_NAME "SDL${VERSION}")
    
    # Try find_package first
    find_package(${SDL_NAME} QUIET)
    
    if(NOT ${SDL_NAME}_FOUND)
        # Try pkg-config
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            string(TOLOWER "${SDL_NAME}" SDL_PKG_NAME)
            pkg_check_modules(${SDL_NAME}_PC QUIET ${SDL_PKG_NAME})
            if(${SDL_NAME}_PC_FOUND)
                set(${SDL_NAME}_FOUND TRUE)
                set(${SDL_NAME}_INCLUDE_DIRS ${${SDL_NAME}_PC_INCLUDE_DIRS})
                set(${SDL_NAME}_LIBRARIES ${${SDL_NAME}_PC_LIBRARIES})
            endif()
        endif()
    endif()
endmacro()