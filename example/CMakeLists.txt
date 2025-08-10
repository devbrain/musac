add_executable(musac_ex
        example.cc
)

add_executable(pc_speaker_example
        pc_speaker_example.cc
)

add_executable(test_mml_decoder
        test_mml_decoder.cc
)

# Link with SDL3 backend if available
if(TARGET musac_backend_sdl3)
    target_link_libraries(musac_ex PRIVATE musac musac_backend_sdl3 musac_test_data)
    target_compile_definitions(musac_ex PRIVATE MUSAC_HAS_SDL3_BACKEND)
    
    target_link_libraries(pc_speaker_example PRIVATE musac musac_backend_sdl3)
    target_compile_definitions(pc_speaker_example PRIVATE MUSAC_HAS_SDL3_BACKEND)
    
    target_link_libraries(test_mml_decoder PRIVATE musac musac_backend_sdl3 musac_test_data)
    target_compile_definitions(test_mml_decoder PRIVATE MUSAC_HAS_SDL3_BACKEND)
else()
    target_link_libraries(musac_ex PRIVATE musac musac_test_data)
    target_link_libraries(pc_speaker_example PRIVATE musac)
    target_link_libraries(test_mml_decoder PRIVATE musac musac_test_data)
endif()

# ImGui player - only build if SDL backend is available
if(TARGET musac_backend_sdl3 OR TARGET musac_backend_sdl2)
    include(FetchContent)
    
    # Fetch ImGui
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.6
    )
    
    FetchContent_MakeAvailable(imgui)
    
    # Create ImGui library with appropriate backend
    add_library(imgui_lib STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    )
    
    target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR})
    
    # Add SDL3 backend if available
    if(TARGET musac_backend_sdl3)
        target_sources(imgui_lib PRIVATE
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
        )
        target_compile_definitions(imgui_lib PUBLIC IMGUI_USE_SDL3)
        # Find SDL3 to link against
        find_package(SDL3 QUIET)
        if(NOT SDL3_FOUND)
            # Use the fetched SDL3 if system SDL3 not found
            if(TARGET SDL3::SDL3-shared)
                target_link_libraries(imgui_lib PUBLIC SDL3::SDL3-shared)
            elseif(TARGET SDL3::SDL3-static)
                target_link_libraries(imgui_lib PUBLIC SDL3::SDL3-static)
            elseif(TARGET SDL3-shared)
                target_link_libraries(imgui_lib PUBLIC SDL3-shared)
            elseif(TARGET SDL3-static)
                target_link_libraries(imgui_lib PUBLIC SDL3-static)
            endif()
        else()
            target_link_libraries(imgui_lib PUBLIC SDL3::SDL3)
        endif()
    # Add SDL2 backend if SDL3 not available but SDL2 is
    elseif(TARGET musac_backend_sdl2)
        target_sources(imgui_lib PRIVATE
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
        )
        target_compile_definitions(imgui_lib PUBLIC IMGUI_USE_SDL2)
        # Find SDL2 to link against
        find_package(SDL2 QUIET)
        if(SDL2_FOUND)
            target_link_libraries(imgui_lib PUBLIC SDL2::SDL2)
        else()
            # Try pkg-config
            find_package(PkgConfig QUIET)
            if(PKG_CONFIG_FOUND)
                pkg_check_modules(SDL2 QUIET sdl2)
                if(SDL2_FOUND)
                    target_include_directories(imgui_lib PUBLIC ${SDL2_INCLUDE_DIRS})
                    target_link_directories(imgui_lib PUBLIC ${SDL2_LIBRARY_DIRS})
                    target_link_libraries(imgui_lib PUBLIC ${SDL2_LIBRARIES})
                endif()
            endif()
        endif()
    endif()
    
    # Build the ImGui player
    add_executable(imgui_player
        imgui_player.cc
    )
    
    if(TARGET musac_backend_sdl3)
        target_link_libraries(imgui_player PRIVATE 
            musac 
            musac_backend_sdl3 
            musac_test_data
            imgui_lib
        )
        target_compile_definitions(imgui_player PRIVATE MUSAC_HAS_SDL3_BACKEND)
    elseif(TARGET musac_backend_sdl2)
        target_link_libraries(imgui_player PRIVATE 
            musac 
            musac_backend_sdl2 
            musac_test_data
            imgui_lib
        )
        target_compile_definitions(imgui_player PRIVATE MUSAC_HAS_SDL2_BACKEND)
    endif()
endif()