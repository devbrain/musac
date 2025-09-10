cmake_minimum_required(VERSION 3.16)

# Include SDL helper module
include(${CMAKE_SOURCE_DIR}/cmake/FindSDLHelper.cmake)

# Only build if SDL backend is available
if((MUSAC_BUILD_SDL3_BACKEND AND TARGET musac_backend_sdl3) OR 
   (MUSAC_BUILD_SDL2_BACKEND AND TARGET musac_backend_sdl2))
   
    # Fetch ImGui if not already available
    if(NOT TARGET imgui_lib)
        include(FetchContent)
        
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.91.6
        )
        
        FetchContent_MakeAvailable(imgui)
        
        # Create ImGui library
        add_library(imgui_lib STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        )
        
        target_include_directories(imgui_lib PUBLIC 
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        
        # Add appropriate SDL backend files
        if(MUSAC_BUILD_SDL3_BACKEND AND TARGET musac_backend_sdl3)
            target_sources(imgui_lib PRIVATE
                ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
            )
            target_compile_definitions(imgui_lib PUBLIC IMGUI_USE_SDL3)
            
            # Use the helper function to find and link SDL3
            find_sdl_library(VERSION 3 TARGET imgui_lib)
            
        elseif(MUSAC_BUILD_SDL2_BACKEND AND TARGET musac_backend_sdl2)
            target_sources(imgui_lib PRIVATE
                ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
            )
            target_compile_definitions(imgui_lib PUBLIC IMGUI_USE_SDL2)
            
            # Use the helper function to find and link SDL2
            find_sdl_library(VERSION 2 TARGET imgui_lib)
        endif()
    endif()
    
    # ImGui Player
    add_executable(imgui_player
        main.cc
        imgui_player.cc
        audio_device_manager.cc
        stream_manager.cc
        waveform_visualizer.cc
        player_ui.cc
    )
    
    # Include directories
    target_include_directories(imgui_player PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/test_data/include
    )
    
    # Core libraries
    target_link_libraries(imgui_player PRIVATE
        musac
        musac_test_data
    )
    
    # Link with appropriate backend and ImGui implementation
    if(MUSAC_BUILD_SDL3_BACKEND AND TARGET musac_backend_sdl3)
        # Use SDL3 backend
        target_link_libraries(imgui_player PRIVATE 
            musac_backend_sdl3
        )
        target_compile_definitions(imgui_player PRIVATE 
            MUSAC_USE_SDL3_BACKEND
            IMGUI_USE_SDL3
        )
        
    elseif(MUSAC_BUILD_SDL2_BACKEND AND TARGET musac_backend_sdl2)
        # Use SDL2 backend
        target_link_libraries(imgui_player PRIVATE 
            musac_backend_sdl2
        )
        target_compile_definitions(imgui_player PRIVATE 
            MUSAC_USE_SDL2_BACKEND
            IMGUI_USE_SDL2
        )
    endif()
    
    # Link with ImGui library (which already includes SDL renderer backends)
    if(TARGET imgui_lib)
        target_link_libraries(imgui_player PRIVATE imgui_lib)
    endif()
    
    # Platform-specific settings
    if(WIN32)
        set_target_properties(imgui_player PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    elseif(APPLE)
        # macOS may need additional frameworks
        find_library(COCOA_LIBRARY Cocoa)
        find_library(IOKIT_LIBRARY IOKit)
        find_library(COREVIDEO_LIBRARY CoreVideo)
        if(COCOA_LIBRARY AND IOKIT_LIBRARY AND COREVIDEO_LIBRARY)
            target_link_libraries(imgui_player PRIVATE
                ${COCOA_LIBRARY}
                ${IOKIT_LIBRARY}
                ${COREVIDEO_LIBRARY}
            )
        endif()
    endif()
    
    # Copy to output directory
    set_target_properties(imgui_player PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    # Installation
    install(TARGETS imgui_player
        RUNTIME DESTINATION bin
    )
    
else()
    message(STATUS "Skipping imgui_player - no SDL backend available")
endif()