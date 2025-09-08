if(MUSAC_BUILD_SDL3_BACKEND)
    set(EXAMPLE_BACKEND "SDL3")
    set(EXAMPLE_BACKEND_TARGET musac_backend_sdl3)
    set(EXAMPLE_BACKEND_DEFINE "MUSAC_USE_SDL3_BACKEND")
elseif(MUSAC_BUILD_SDL2_BACKEND)
    set(EXAMPLE_BACKEND "SDL2")
    set(EXAMPLE_BACKEND_TARGET musac_backend_sdl2)
    set(EXAMPLE_BACKEND_DEFINE "MUSAC_USE_SDL2_BACKEND")
else()
    message(WARNING "No audio backend enabled. Examples will not be built.")
    return()
endif()

message(STATUS "Examples will use ${EXAMPLE_BACKEND} backend")

# Helper function to add an example
function(add_musac_example NAME)
    add_executable(${NAME} ${NAME}.cc)
    target_link_libraries(${NAME} PRIVATE musac musac_codecs ${EXAMPLE_BACKEND_TARGET} musac_test_data)
    target_compile_definitions(${NAME} PRIVATE ${EXAMPLE_BACKEND_DEFINE})
endfunction()

# Add all examples
add_musac_example(01_play_file)
add_musac_example(02_volume_control)
add_musac_example(03_looping)
add_musac_example(04_pc_speaker)
add_musac_example(05_device_enumeration)

add_musac_example(musac_example)
add_musac_example(pc_speaker_example)
add_musac_example(test_mml_decoder)


add_subdirectory(imgui_player)