add_executable(musac_ex
        example.cc
        loader.cc
        loader.hh

        data/cmf_example.h
        data/hmp_example.h
        data/mid_example.h
        data/xmi_example.h
        data/mp3_example.h
        data/opb_example.h
        data/s3m_example.h
        data/voc_example.h
        data/xmi_example.h
)
fetchcontent_declare(
                MAGIC_ENUM
                GIT_REPOSITORY "https://github.com/Neargye/magic_enum.git"
                GIT_TAG v0.9.7
                GIT_PROGRESS TRUE
                UPDATE_COMMAND ""
                OVERRIDE_FIND_PACKAGE
        )
fetchcontent_makeavailable(MAGIC_ENUM)

target_include_directories(musac_ex PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(musac_ex PRIVATE musac magic_enum)