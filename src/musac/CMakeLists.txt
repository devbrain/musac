set (thirdparty
        codecs/dr_libs/dr_wav.h
        codecs/dr_libs/dr_flac.h
        codecs/dr_libs/dr_mp3.h

        codecs/libmodplug/fastmix.c
        codecs/libmodplug/libmodplug.h
        codecs/libmodplug/load_669.c
        codecs/libmodplug/load_amf.c
        codecs/libmodplug/load_ams.c
        codecs/libmodplug/load_dbm.c
        codecs/libmodplug/load_dmf.c
        codecs/libmodplug/load_dsm.c
        codecs/libmodplug/load_far.c
        codecs/libmodplug/load_gdm.c
        codecs/libmodplug/load_it.c
        codecs/libmodplug/load_mdl.c
        codecs/libmodplug/load_med.c
        codecs/libmodplug/load_mod.c
        codecs/libmodplug/load_mt2.c
        codecs/libmodplug/load_mtm.c
        codecs/libmodplug/load_okt.c
        codecs/libmodplug/load_psm.c
        codecs/libmodplug/load_ptm.c
        codecs/libmodplug/load_s3m.c
        codecs/libmodplug/load_stm.c
        codecs/libmodplug/load_ult.c
        codecs/libmodplug/load_umx.c
        codecs/libmodplug/load_xm.c
        codecs/libmodplug/mmcmp.c
        codecs/libmodplug/modplug.c
        codecs/libmodplug/modplug.h
        codecs/libmodplug/snd_dsp.c
        codecs/libmodplug/snd_flt.c
        codecs/libmodplug/snd_fx.c
        codecs/libmodplug/sndfile.c
        codecs/libmodplug/sndmix.c
        codecs/libmodplug/tables.h

        codecs/opb_lib/opblib.c
        codecs/opb_lib/opblib.h

        codecs/opl/adlib_emu.cpp
        codecs/opl/adlib_emu.h
        codecs/opl/impl_opl.c
        codecs/opl/impl_opl.h
        codecs/opl/opl.cc
        codecs/opl/opl.hh
        codecs/opl/opl_command.h
        codecs/opl/opl_player.cc
        codecs/opl/opl_player.hh
        codecs/opl/ymfm/ymfm.h
        codecs/opl/ymfm/ymfm_adpcm.cpp
        codecs/opl/ymfm/ymfm_adpcm.h
        codecs/opl/ymfm/ymfm_fm.h
        codecs/opl/ymfm/ymfm_fm.ipp
        codecs/opl/ymfm/ymfm_misc.cpp
        codecs/opl/ymfm/ymfm_misc.h
        codecs/opl/ymfm/ymfm_opl.cpp
        codecs/opl/ymfm/ymfm_opl.h
        codecs/opl/ymfm/ymfm_opm.cpp
        codecs/opl/ymfm/ymfm_opm.h
        codecs/opl/ymfm/ymfm_opn.cpp
        codecs/opl/ymfm/ymfm_opn.h
        codecs/opl/ymfm/ymfm_opq.cpp
        codecs/opl/ymfm/ymfm_opq.h
        codecs/opl/ymfm/ymfm_opz.cpp
        codecs/opl/ymfm/ymfm_opz.h
        codecs/opl/ymfm/ymfm_pcm.cpp
        codecs/opl/ymfm/ymfm_pcm.h
        codecs/opl/ymfm/ymfm_ssg.cpp
        codecs/opl/ymfm/ymfm_ssg.h
        codecs/opl/ymfm_chip.cc
        codecs/opl/ymfm_chip.hh
        codecs/opl/ymfm/ymfm_disable_warnings.h
)

set (sdk_sources
        ${MUSAC_PROJECT_ROOT}/musac/sdk/proc_decoder.hh
        sdk/proc_decoder.cc

        ${MUSAC_PROJECT_ROOT}/musac/sdk/samples_converter.hh
        sdk/samples_converter.cc

        ${MUSAC_PROJECT_ROOT}/musac/sdk/buffer.hh
        ${MUSAC_PROJECT_ROOT}/musac/sdk/decoder.hh
        sdk/decoder.cc

        sdk/processor.cc
        ${MUSAC_PROJECT_ROOT}/musac/sdk/processor.hh

        sdk/resampler.cc
        ${MUSAC_PROJECT_ROOT}/musac/sdk/resampler.hh
        sdk/from_float_converter.cc
        ${MUSAC_PROJECT_ROOT}/musac/sdk/from_float_converter.hh
)

set(codecs_sources
        codecs/decoder_aiff.cc
        ${MUSAC_PROJECT_ROOT}/musac/codecs/decoder_aiff.hh
        codecs/decoder_cmf.cc
        ${MUSAC_PROJECT_ROOT}/musac/codecs/decoder_cmf.hh
        codecs/decoder_drflac.cc
        ${MUSAC_PROJECT_ROOT}/musac/codecs/decoder_drflac.hh
        codecs/decoder_drmp3.cc
        ${MUSAC_PROJECT_ROOT}/musac/codecs/decoder_drmp3.hh
        codecs/decoder_drwav.cc
        ${MUSAC_PROJECT_ROOT}/musac/codecs/decoder_drwav.hh
        codecs/decoder_modplug.cc
        ${MUSAC_PROJECT_ROOT}/musac/codecs/decoder_modplug.hh
        codecs/decoder_opb.cc
        ${MUSAC_PROJECT_ROOT}/musac/codecs/decoder_opb.hh
        codecs/decoder_seq.cc
        ${MUSAC_PROJECT_ROOT}/musac/codecs/decoder_seq.hh
        codecs/decoder_vgm.cc
        ${MUSAC_PROJECT_ROOT}/musac/codecs/decoder_vgm.hh
        codecs/decoder_voc.cc
        ${MUSAC_PROJECT_ROOT}/musac/codecs/decoder_voc.hh


        codecs/aiff/load_aiff.c
        codecs/aiff/load_aiff.h

        codecs/cmf/fmdrv.c
        codecs/cmf/fmdrv.h



        codecs/seq/midi_opl.h
        codecs/seq/patches.cpp
        codecs/seq/patches.h
        codecs/seq/patchnames.cpp
        codecs/seq/player.cpp
        codecs/seq/player.h
        codecs/seq/sequence.cpp
        codecs/seq/sequence.h
        codecs/seq/sequence_hmi.cpp
        codecs/seq/sequence_hmi.h
        codecs/seq/sequence_hmp.cpp
        codecs/seq/sequence_hmp.h
        codecs/seq/sequence_mid.cpp
        codecs/seq/sequence_mid.h
        codecs/seq/sequence_mus.cpp
        codecs/seq/sequence_mus.h
        codecs/seq/sequence_xmi.cpp
        codecs/seq/sequence_xmi.h

        codecs/vgm/em_inflate.cpp
        codecs/vgm/em_inflate.h
        codecs/vgm/vgm_player.cc
        codecs/vgm/vgm_player.hh

        codecs/voc/load_voc.c
        codecs/voc/load_voc.h

        ${thirdparty}
)

add_library(musac STATIC
        audio_device.cc
        ${MUSAC_PROJECT_ROOT}/musac/audio_device.hh
        audio_system.cc
        ${MUSAC_PROJECT_ROOT}/musac/audio_system.hh
        audio_source.cc
        ${MUSAC_PROJECT_ROOT}/musac/audio_source.hh
        audio_loader.cc
        ${MUSAC_PROJECT_ROOT}/musac/audio_loader.hh



        lock.hh
        mutex.cc
        mutex.hh


        stream.cc
        ${MUSAC_PROJECT_ROOT}/musac/stream.hh

        fade_envelop.cc
        fade_envelop.hh
        in_use_guard.cc
        in_use_guard.hh
        callback_dispatcher.cc
        callback_dispatcher.hh
        audio_mixer.cc
        audio_mixer.hh
        ${MUSAC_PROJECT_ROOT}/musac/audio_device_data.hh


        resampler/resampler_sdl.cc
        resampler/resampler_sdl.hh
        resampler/resampler_speex.cc
        resampler/resampler_speex.hh
        resampler/speex/arch.h
        resampler/speex/resample.c
        resampler/speex/resample_sse.h
        resampler/speex/speex_resampler.h

        ${sdk_sources}
        ${codecs_sources}

)

target_link_libraries(musac PUBLIC SDL3::SDL3)
target_include_directories(musac
        PUBLIC ${MUSAC_PROJECT_ROOT}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

