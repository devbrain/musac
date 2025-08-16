set(thirdparty
        dr_libs/dr_wav.h
        dr_libs/dr_flac.h
        dr_libs/dr_mp3.h

        libmodplug/fastmix.c
        libmodplug/libmodplug.h
        libmodplug/load_669.c
        libmodplug/load_amf.c
        libmodplug/load_ams.c
        libmodplug/load_dbm.c
        libmodplug/load_dmf.c
        libmodplug/load_dsm.c
        libmodplug/load_far.c
        libmodplug/load_gdm.c
        libmodplug/load_it.c
        libmodplug/load_mdl.c
        libmodplug/load_med.c
        libmodplug/load_mod.c
        libmodplug/load_mt2.c
        libmodplug/load_mtm.c
        libmodplug/load_okt.c
        libmodplug/load_psm.c
        libmodplug/load_ptm.c
        libmodplug/load_s3m.c
        libmodplug/load_stm.c
        libmodplug/load_ult.c
        libmodplug/load_umx.c
        libmodplug/load_xm.c
        libmodplug/mmcmp.c
        libmodplug/modplug.c
        libmodplug/modplug.h
        libmodplug/snd_dsp.c
        libmodplug/snd_flt.c
        libmodplug/snd_fx.c
        libmodplug/sndfile.c
        libmodplug/sndmix.c
        libmodplug/tables.h
        opb_lib/opblib.c
        opb_lib/opblib.h
        vorbis/stb_vorbis.c
)

set(CODECS_INCLUDE ${MUSAC_PROJECT_ROOT}/include/musac/codecs/)

set(codecs_sources
        decoder_aiff.cc
        ${CODECS_INCLUDE}decoder_aiff.hh
        
        # AIFF refactored components
        aiff/aiff_container.cc
        ${CODECS_INCLUDE}aiff/aiff_container.hh
        ${CODECS_INCLUDE}aiff/aiff_codec_base.hh
        aiff/aiff_codec_factory.cc
        ${CODECS_INCLUDE}aiff/aiff_codec_factory.hh
        aiff/codecs/pcm_codec.cc
        aiff/codecs/ima4_codec.cc
        aiff/codecs/float_codec.cc
        aiff/codecs/g711_codec.cc
        
        decoder_8svx.cc
        ${CODECS_INCLUDE}decoder_8svx.hh
        decoder_cmf.cc
        ${CODECS_INCLUDE}decoder_cmf.hh
        decoder_drflac.cc
        ${CODECS_INCLUDE}decoder_drflac.hh
        decoder_drmp3.cc
        ${CODECS_INCLUDE}decoder_drmp3.hh
        decoder_drwav.cc
        ${CODECS_INCLUDE}decoder_drwav.hh
        decoder_modplug.cc
        ${CODECS_INCLUDE}decoder_modplug.hh
        decoder_opb.cc
        ${CODECS_INCLUDE}decoder_opb.hh
        decoder_seq.cc
        ${CODECS_INCLUDE}decoder_seq.hh
        decoder_vgm.cc
        ${CODECS_INCLUDE}decoder_vgm.hh
        decoder_voc.cc
        ${CODECS_INCLUDE}decoder_voc.hh
        decoder_vorbis.cc
        ${CODECS_INCLUDE}decoder_vorbis.hh
        decoder_mml.cc
        ${CODECS_INCLUDE}decoder_mml.hh
        register_codecs.cc
        ${CODECS_INCLUDE}register_codecs.hh


        cmf/fmdrv.c
        cmf/fmdrv.h



        vgm/em_inflate.cpp
        vgm/em_inflate.h
        vgm/vgm_player.cc
        vgm/vgm_player.hh
)

if (MUSAC_BUILD_FPIC)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

if (MUSAC_BUILD_SHARED)
    set(LIB_BUILD_TYPE SHARED)
else ()
    set(LIB_BUILD_TYPE STATIC)
endif ()

set(LIBIFF_BUILD_SHARED ${MUSAC_BUILD_SHARED} CACHE INTERNAL "")
set(LIBIFF_BUILD_FPIC ${MUSAC_BUILD_FPIC} CACHE INTERNAL "")
set(LIBIFF_BUILD_DOCUMENTATION OFF CACHE INTERNAL "")
set(LIBIFF_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(LIBIFF_BUILD_TESTS OFF CACHE INTERNAL "")

fetchcontent_declare(
        LIBIFF_EXT
        GIT_REPOSITORY "https://github.com/devbrain/libiff.git"
        GIT_TAG master
        GIT_PROGRESS TRUE
        UPDATE_COMMAND ""
        OVERRIDE_FIND_PACKAGE
)

fetchcontent_makeavailable(LIBIFF_EXT)

# Include Kaitai support
list(APPEND CMAKE_MODULE_PATH ${MUSAC_PROJECT_ROOT}/cmake)
include(FindKaitai)

# Generate C++ from Kaitai specifications (required for AIFF v3)
if(NOT KAITAI_FOUND)
    message(FATAL_ERROR "Kaitai compiler not found. It is required for AIFF v3 decoder.")
endif()

# Generate AIFF chunk parsers
kaitai_generate_cpp(KAITAI_AIFF_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/aiff/kaitai/aiff_chunks.ksy
    NAMESPACE musac_kaitai
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/kaitai_generated
)

# Add generated sources to codec sources
list(APPEND codecs_sources ${KAITAI_AIFF_SOURCES})

# Create a library for Kaitai-generated parsers
add_library(musac_kaitai_parsers OBJECT ${KAITAI_AIFF_SOURCES})
target_link_libraries(musac_kaitai_parsers PUBLIC Kaitai::Runtime)
target_include_directories(musac_kaitai_parsers PUBLIC 
    ${CMAKE_CURRENT_BINARY_DIR}/kaitai_generated
)

message(STATUS "Kaitai support enabled for AIFF decoder")

add_library(musac_codecs ${LIB_BUILD_TYPE}
        ${codecs_sources}
        ${thirdparty}
)

target_include_directories(musac_codecs
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
        $<BUILD_INTERFACE:${MUSAC_PROJECT_ROOT}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(musac_codecs
        PUBLIC
        musac_sdk
        thirdparty::failsafe
        PRIVATE
        iff
)

# Link Kaitai runtime (always required)
target_link_libraries(musac_codecs PRIVATE Kaitai::Runtime)
target_include_directories(musac_codecs PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}/kaitai_generated
)

# Set logger category for codecs
target_compile_definitions(musac_codecs PRIVATE LOGGER_DEFAULT_CATEGORY=musac::codecs)

# Detect endianness at configure time
include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    target_compile_definitions(musac_codecs PRIVATE MUSAC_BIG_ENDIAN=1)
else()
    target_compile_definitions(musac_codecs PRIVATE MUSAC_LITTLE_ENDIAN=1)
endif()

if (MUSAC_BUILD_SHARED)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

    include(GenerateExportHeader)
    set(export_file_name "export_musac_codecs.h")

    generate_export_header(musac_codecs EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/musac/codecs/${export_file_name})
endif ()