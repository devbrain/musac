# MUSAC Test Data Library
# Internal static library for testing and demos

# Fetch magic_enum dependency
fetchcontent_declare(
        MAGIC_ENUM
        GIT_REPOSITORY "https://github.com/Neargye/magic_enum.git"
        GIT_TAG v0.9.7
        GIT_PROGRESS TRUE
        UPDATE_COMMAND ""
        OVERRIDE_FIND_PACKAGE
)
fetchcontent_makeavailable(MAGIC_ENUM)

# Create the test data library
add_library(musac_test_data STATIC
        src/loader.cc
        include/musac/test_data/loader.hh

        # Include data files for IDE visibility
        data/cmf_example.h
        data/hmp_example.h
        data/mid_example.h
        data/mml_bouree.h
        data/mml_complex.h
        data/mp3_example.h
        data/opb_example.h
        data/s3m_example.h
        data/voc_example.h
        data/xmi_example.h
        data/ogg_example.h
)

# Set include directories
target_include_directories(musac_test_data
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(musac_test_data
        PUBLIC
        musac
        musac_codecs
        PRIVATE
        magic_enum
)

# Set properties
set_target_properties(musac_test_data PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ${MUSAC_BUILD_FPIC}
)

# Apply warning flags
if (MUSAC_WARNING_FLAGS)
    target_compile_options(musac_test_data PRIVATE ${MUSAC_WARNING_FLAGS})
endif ()

# This is an internal library, don't install it
set_target_properties(musac_test_data PROPERTIES
        EXCLUDE_FROM_ALL TRUE
)

# Add alias for consistency
add_library(musac::test_data ALIAS musac_test_data)