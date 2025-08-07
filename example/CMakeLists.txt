add_executable(musac_ex
        example.cc
)

# Link with SDL3 backend if available
if(TARGET musac_backend_sdl3)
    target_link_libraries(musac_ex PRIVATE musac musac_backend_sdl3 musac_test_data)
    target_compile_definitions(musac_ex PRIVATE MUSAC_HAS_SDL3_BACKEND)
else()
    target_link_libraries(musac_ex PRIVATE musac musac_test_data)
endif()