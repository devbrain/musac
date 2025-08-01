cmake_minimum_required(VERSION 3.16)

add_executable(generate_test_data generate_test_data.cc)

target_link_libraries(generate_test_data 
    PRIVATE 
        musac_codecs
        musac_sdk
        SDL3::SDL3
        thirdparty::failsafe
)

target_include_directories(generate_test_data 
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
)

# Set output directory
set_target_properties(generate_test_data PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools
)