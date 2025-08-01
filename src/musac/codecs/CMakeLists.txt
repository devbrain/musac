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
)

set(CODECS_INCLUDE ${MUSAC_PROJECT_ROOT}/musac/codecs/)

set(codecs_sources
        decoder_aiff.cc
        ${CODECS_INCLUDE}decoder_aiff.hh
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


        cmf/fmdrv.c
        cmf/fmdrv.h


        seq/midi_opl.h
        seq/patches.cpp
        seq/patches.h
        seq/patchnames.cpp
        seq/player.cpp
        seq/player.h
        seq/sequence.cpp
        seq/sequence.h
        seq/sequence_hmi.cpp
        seq/sequence_hmi.h
        seq/sequence_hmp.cpp
        seq/sequence_hmp.h
        seq/sequence_mid.cpp
        seq/sequence_mid.h
        seq/sequence_mus.cpp
        seq/sequence_mus.h
        seq/sequence_xmi.cpp
        seq/sequence_xmi.h

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
target_link_libraries(musac_codecs PUBLIC musac_sdk thirdparty::failsafe)

if (MUSAC_BUILD_SHARED)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

    include(GenerateExportHeader)
    set(export_file_name "export_musac_codecs.h")

    generate_export_header(musac_codecs EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/musac/codecs/${export_file_name})
endif ()