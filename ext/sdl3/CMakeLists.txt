# First try to find SDL3 locally
find_package(SDL3 QUIET)

if(NOT SDL3_FOUND)
    message(STATUS "SDL3 not found locally, fetching from GitHub...")
    
    set(SDL_VERSION "3.2.16")
    
    # Configure SDL3 build type based on SDLPP_BUILD_SHARED
    if(SDLPP_BUILD_SHARED)
        set(SDL_STATIC OFF CACHE INTERNAL "")
        set(SDL_SHARED ON CACHE INTERNAL "")
    else()
        set(SDL_STATIC ON CACHE INTERNAL "")
        set(SDL_SHARED OFF CACHE INTERNAL "")
    endif()
    
    set(SDL_LIBURING OFF CACHE INTERNAL "")
    # SDL_INSTALL must be ON so SDL3 can be found by downstream users
    set(SDL_INSTALL ON CACHE INTERNAL "")
    # Disable SDL tests to prevent CTest targets from being created
    set(SDL_TESTS OFF CACHE INTERNAL "")
    set(SDL_EXAMPLES OFF CACHE INTERNAL "")
    
    fetchcontent_declare(
            SDL3
            URL https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL3-${SDL_VERSION}.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            TIMEOUT 10
            UPDATE_COMMAND ""
            OVERRIDE_FIND_PACKAGE
    )
    
    fetchcontent_makeavailable(SDL3)
else()
    message(STATUS "Found SDL3 locally: ${SDL3_VERSION}")
endif()

