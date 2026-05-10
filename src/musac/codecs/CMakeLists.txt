# MUSAC Codecs Library
cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Third-party codec sources (bundled)
# ============================================================================

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

# ============================================================================
# Codec Sources
# ============================================================================

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

    # Common codec components shared between decoders
    common/g711_codec.cc
    ${CODECS_INCLUDE}common/g711_codec.hh
    common/adpcm_codec.cc
    ${CODECS_INCLUDE}common/adpcm_codec.hh

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

# ============================================================================
# Build Configuration
# ============================================================================

if(MUSAC_BUILD_FPIC)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(MUSAC_BUILD_SHARED)
    set(LIB_BUILD_TYPE SHARED)
else()
    set(LIB_BUILD_TYPE STATIC)
endif()

# ============================================================================
# Dependencies - libiff (using neutrino-cmake)
# ============================================================================

include(${NEUTRINO_CMAKE_DIR}/deps/libiff.cmake)

# Configure libiff options to match our build
set(NEUTRINO_LIBIFF_BUILD_SHARED ${MUSAC_BUILD_SHARED} CACHE INTERNAL "")

include(FetchContent)
find_package(Git REQUIRED)

# neutrino-cmake currently fetches libiff from master. Predeclare the same
# content so CMake applies our compatibility patch before building it.
FetchContent_Declare(libiff
    GIT_REPOSITORY https://github.com/devbrain/libiff.git
    GIT_TAG ${NEUTRINO_LIBIFF_VERSION}
    GIT_SHALLOW TRUE
    PATCH_COMMAND
        ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
        ${GIT_EXECUTABLE} apply --ignore-whitespace
        ${MUSAC_PROJECT_ROOT}/cmake/patches/libiff-chunk-reader-size-t.patch
)

neutrino_fetch_libiff()

# ============================================================================
# DataScript Support (replaces Kaitai)
# ============================================================================

include(${NEUTRINO_CMAKE_DIR}/deps/datascript.cmake)

# Enable host tools build (ds compiler)
set(NEUTRINO_DATASCRIPT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NEUTRINO_DATASCRIPT_BUILD_HOST_TOOLS ON CACHE BOOL "" FORCE)

neutrino_fetch_datascript()

# Generate parsers from datascript schemas
datascript_generate(
    TARGET musac_parsers
    SCHEMAS
        ${CMAKE_CURRENT_SOURCE_DIR}/formats/aiff.ds
        ${CMAKE_CURRENT_SOURCE_DIR}/formats/esvx.ds
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/datascript_gen
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    PRESERVE_PACKAGE_DIRS ON
)

message(STATUS "DataScript support enabled for AIFF and 8SVX decoders")

# ============================================================================
# Create Library
# ============================================================================

add_library(musac_codecs ${LIB_BUILD_TYPE}
    ${codecs_sources}
    ${thirdparty}
)
add_library(musac::codecs ALIAS musac_codecs)

target_include_directories(musac_codecs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
        $<BUILD_INTERFACE:${MUSAC_PROJECT_ROOT}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(musac_codecs
    PUBLIC
        musac_sdk
        thirdparty::failsafe
    PRIVATE
        neutrino::iff
        musac_parsers
)

# Set logger category for codecs
target_compile_definitions(musac_codecs PRIVATE LOGGER_DEFAULT_CATEGORY=musac::codecs)

# ============================================================================
# Endianness Detection
# ============================================================================

include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    target_compile_definitions(musac_codecs PRIVATE MUSAC_BIG_ENDIAN=1)
else()
    target_compile_definitions(musac_codecs PRIVATE MUSAC_LITTLE_ENDIAN=1)
endif()

# ============================================================================
# Export Header
# ============================================================================

include(GenerateExportHeader)
set(export_file_name "export_musac_codecs.h")

generate_export_header(musac_codecs
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/musac/codecs/${export_file_name}
    STATIC_DEFINE MUSAC_CODECS_STATIC_DEFINE
)

if(MUSAC_BUILD_SHARED)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
else()
    target_compile_definitions(musac_codecs PUBLIC MUSAC_CODECS_STATIC_DEFINE)
endif()

# Warning flags
if(DEFINED MUSAC_WARNING_FLAGS)
    target_compile_options(musac_codecs PRIVATE ${MUSAC_WARNING_FLAGS})
endif()

# Suppress warnings for third-party code
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(
        # Files that include dr_libs headers (header-only libraries)
        decoder_drwav.cc
        decoder_drflac.cc
        decoder_drmp3.cc
        # Other third-party code
        vorbis/stb_vorbis.c
        opb_lib/opblib.c
        cmf/fmdrv.c
        vgm/em_inflate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/fastmix.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_669.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_amf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_ams.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_dbm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_dmf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_dsm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_far.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_gdm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_it.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_mdl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_med.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_mod.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_mt2.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_mtm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_okt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_psm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_ptm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_s3m.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_stm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_ult.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_umx.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_xm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/mmcmp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/modplug.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/snd_dsp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/snd_flt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/snd_fx.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/sndfile.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/sndmix.c
        PROPERTIES COMPILE_OPTIONS "-w"
    )
elseif(MSVC)
    set_source_files_properties(
        # Files that include dr_libs headers (header-only libraries)
        decoder_drwav.cc
        decoder_drflac.cc
        decoder_drmp3.cc
        # Other third-party code
        vorbis/stb_vorbis.c
        opb_lib/opblib.c
        cmf/fmdrv.c
        vgm/em_inflate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/fastmix.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_669.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_amf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_ams.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_dbm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_dmf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_dsm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_far.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_gdm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_it.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_mdl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_med.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_mod.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_mt2.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_mtm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_okt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_psm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_ptm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_s3m.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_stm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_ult.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_umx.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/load_xm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/mmcmp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/modplug.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/snd_dsp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/snd_flt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/snd_fx.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/sndfile.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmodplug/sndmix.c
        PROPERTIES COMPILE_FLAGS "/W0"
    )
endif()

# ============================================================================
# Installation
# ============================================================================

if(NOT MUSAC_IS_SUBDIRECTORY AND NEUTRINO_MUSAC_INSTALL)
    install(TARGETS musac_codecs
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install codec headers
    install(DIRECTORY ${MUSAC_PROJECT_ROOT}/include/musac/codecs
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/musac
        FILES_MATCHING PATTERN "*.hh" PATTERN "*.h"
    )

    # Install generated export header if shared build
    if(MUSAC_BUILD_SHARED)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/musac/codecs/export_musac_codecs.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/musac/codecs
        )
    endif()
endif()
