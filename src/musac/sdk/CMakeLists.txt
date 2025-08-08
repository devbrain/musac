set (sdk_sources
        ${MUSAC_PROJECT_ROOT}/musac/sdk/samples_converter.hh
        samples_converter.cc

        ${MUSAC_PROJECT_ROOT}/musac/sdk/buffer.hh
        ${MUSAC_PROJECT_ROOT}/musac/sdk/decoder.hh
        decoder.cc

        processor.cc
        ${MUSAC_PROJECT_ROOT}/musac/sdk/processor.hh

        resampler.cc
        ${MUSAC_PROJECT_ROOT}/musac/sdk/resampler.hh
        from_float_converter.cc
        ${MUSAC_PROJECT_ROOT}/musac/sdk/from_float_converter.hh
        
        # New core headers and sources
        ${MUSAC_PROJECT_ROOT}/musac/sdk/types.hh
        ${MUSAC_PROJECT_ROOT}/musac/sdk/audio_format.hh
        audio_format.cc
        ${MUSAC_PROJECT_ROOT}/musac/sdk/io_stream.hh
        ${MUSAC_PROJECT_ROOT}/musac/sdk/endian.hh
        ${MUSAC_PROJECT_ROOT}/musac/sdk/audio_converter_v2.hh
        ${CMAKE_CURRENT_BINARY_DIR}/musac/sdk/musac_sdk_config.h
        io_stream.cc
        audio_converter_v2.cc
        audio_converter_internal.cc
)

set(opl_sources
        opl/adlib_emu.cpp
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/adlib_emu.h
        opl/impl_opl.c
        opl/impl_opl.h
        opl/opl.cc
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/opl.hh
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/opl_command.h
        opl/opl_player.cc
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/opl_player.hh
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm.h
        opl/ymfm/ymfm_adpcm.cpp
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_adpcm.h
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_fm.h
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_fm.ipp
        opl/ymfm/ymfm_misc.cpp
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_misc.h
        opl/ymfm/ymfm_opl.cpp
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_opl.h
        opl/ymfm/ymfm_opm.cpp
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_opm.h
        opl/ymfm/ymfm_opn.cpp
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_opn.h
        opl/ymfm/ymfm_opq.cpp
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_opq.h
        opl/ymfm/ymfm_opz.cpp
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_opz.h
        opl/ymfm/ymfm_pcm.cpp
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_pcm.h
        opl/ymfm/ymfm_ssg.cpp
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm/ymfm_ssg.h
        opl/ymfm_chip.cc
        ${MUSAC_PROJECT_ROOT}/musac/sdk/opl/ymfm_chip.hh
)

# Test for endianness
include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)

# Generate configuration header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/musac_sdk_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/musac/sdk/musac_sdk_config.h
)

if (MUSAC_BUILD_FPIC)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if (MUSAC_BUILD_SHARED)
    set(LIB_BUILD_TYPE SHARED)
else()
    set(LIB_BUILD_TYPE STATIC)
endif()

add_library(musac_sdk ${LIB_BUILD_TYPE}
        ${sdk_sources}
        ${opl_sources}
)

target_include_directories(musac_sdk
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
            $<BUILD_INTERFACE:${MUSAC_PROJECT_ROOT}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(musac_sdk PUBLIC thirdparty::failsafe)

# Set logger category for sdk
target_compile_definitions(musac_sdk PRIVATE LOGGER_DEFAULT_CATEGORY=musac::sdk)

if (MUSAC_BUILD_SHARED)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

    include(GenerateExportHeader)
    set(export_file_name "export_musac_sdk.h")

    generate_export_header(musac_sdk EXPORT_FILE_NAME  ${CMAKE_CURRENT_BINARY_DIR}/musac/sdk/${export_file_name})
endif()