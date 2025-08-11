add_executable(musac_ex
        example.cc
)

add_executable(pc_speaker_example
        pc_speaker_example.cc
)

add_executable(test_mml_decoder
        test_mml_decoder.cc
)

# Link with appropriate SDL backend based on build options
if(MUSAC_BUILD_SDL3_BACKEND AND TARGET musac_backend_sdl3)
    target_link_libraries(musac_ex PRIVATE musac musac_backend_sdl3 musac_test_data)
    target_compile_definitions(musac_ex PRIVATE MUSAC_HAS_SDL3_BACKEND)
    
    target_link_libraries(pc_speaker_example PRIVATE musac musac_backend_sdl3)
    target_compile_definitions(pc_speaker_example PRIVATE MUSAC_HAS_SDL3_BACKEND)
    
    target_link_libraries(test_mml_decoder PRIVATE musac musac_backend_sdl3 musac_test_data)
    target_compile_definitions(test_mml_decoder PRIVATE MUSAC_HAS_SDL3_BACKEND)
elseif(MUSAC_BUILD_SDL2_BACKEND AND TARGET musac_backend_sdl2)
    target_link_libraries(musac_ex PRIVATE musac musac_backend_sdl2 musac_test_data)
    target_compile_definitions(musac_ex PRIVATE MUSAC_HAS_SDL2_BACKEND)
    
    target_link_libraries(pc_speaker_example PRIVATE musac musac_backend_sdl2)
    target_compile_definitions(pc_speaker_example PRIVATE MUSAC_HAS_SDL2_BACKEND)
    
    target_link_libraries(test_mml_decoder PRIVATE musac musac_backend_sdl2 musac_test_data)
    target_compile_definitions(test_mml_decoder PRIVATE MUSAC_HAS_SDL2_BACKEND)
else()
    target_link_libraries(musac_ex PRIVATE musac musac_test_data)
    target_link_libraries(pc_speaker_example PRIVATE musac)
    target_link_libraries(test_mml_decoder PRIVATE musac musac_test_data)
endif()

add_subdirectory(imgui_player)