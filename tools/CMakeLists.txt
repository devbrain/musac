cmake_minimum_required(VERSION 3.16)

add_executable(generate_test_data generate_test_data.cc)

target_link_libraries(generate_test_data 
    PRIVATE 
        musac_codecs
        musac_sdk
        thirdparty::failsafe
)

# Link with appropriate SDL version based on what's being built
if(MUSAC_BUILD_SDL3_BACKEND AND TARGET musac_backend_sdl3)
    target_link_libraries(generate_test_data PRIVATE musac_backend_sdl3)
elseif(MUSAC_BUILD_SDL2_BACKEND AND TARGET musac_backend_sdl2)
    target_link_libraries(generate_test_data PRIVATE musac_backend_sdl2)
else()
    message(WARNING "generate_test_data tool requires an SDL backend but none is being built")
endif()

target_include_directories(generate_test_data 
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
)

# Set output directory
set_target_properties(generate_test_data PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools
)