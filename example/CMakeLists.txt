add_executable(musac_ex
        example.cc
)
target_link_libraries(musac_ex PRIVATE musac musac_test_data)