cmake_minimum_required(VERSION 3.20)

# Only build if SDL backend is available
if((MUSAC_BUILD_SDL3_BACKEND AND TARGET musac_backend_sdl3) OR
   (MUSAC_BUILD_SDL2_BACKEND AND TARGET musac_backend_sdl2))

    # Use neutrino-cmake imgui recipe
    include(${NEUTRINO_CMAKE_DIR}/deps/imgui.cmake)

    # Fetch ImGui with appropriate backend
    if(MUSAC_BUILD_SDL3_BACKEND AND TARGET musac_backend_sdl3)
        # SDL3 is already available from backend, just enable imgui backends
        set(NEUTRINO_IMGUI_BACKEND_SDL3 ON CACHE BOOL "" FORCE)
        set(NEUTRINO_IMGUI_BACKEND_SDLRENDERER3 ON CACHE BOOL "" FORCE)
        neutrino_fetch_imgui()
    elseif(MUSAC_BUILD_SDL2_BACKEND AND TARGET musac_backend_sdl2)
        # SDL2 is already available from backend, just enable imgui backends
        set(NEUTRINO_IMGUI_BACKEND_SDL2 ON CACHE BOOL "" FORCE)
        set(NEUTRINO_IMGUI_BACKEND_SDLRENDERER2 ON CACHE BOOL "" FORCE)
        neutrino_fetch_imgui()
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
        imgui::imgui
    )

    # Link with appropriate backend and ImGui implementation
    if(MUSAC_BUILD_SDL3_BACKEND AND TARGET musac_backend_sdl3)
        target_link_libraries(imgui_player PRIVATE
            musac_backend_sdl3
            imgui::backend_sdl3
            imgui::backend_sdlrenderer3
        )
        target_compile_definitions(imgui_player PRIVATE
            MUSAC_USE_SDL3_BACKEND
            IMGUI_USE_SDL3
        )
    elseif(MUSAC_BUILD_SDL2_BACKEND AND TARGET musac_backend_sdl2)
        target_link_libraries(imgui_player PRIVATE
            musac_backend_sdl2
            imgui::backend_sdl2
            imgui::backend_sdlrenderer2
        )
        target_compile_definitions(imgui_player PRIVATE
            MUSAC_USE_SDL2_BACKEND
            IMGUI_USE_SDL2
        )
    endif()

    # Platform-specific settings
    if(WIN32)
        set_target_properties(imgui_player PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    elseif(APPLE)
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
    if(NEUTRINO_MUSAC_INSTALL)
        install(TARGETS imgui_player RUNTIME DESTINATION bin)
    endif()

else()
    message(STATUS "Skipping imgui_player - no SDL backend available")
endif()
