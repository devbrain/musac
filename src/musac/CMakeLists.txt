
add_subdirectory(sdk)
add_subdirectory(codecs)

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

        ${codecs_sources}

)

target_link_libraries(musac PUBLIC SDL3::SDL3 musac_codecs musac_sdk thirdparty::failsafe)
target_include_directories(musac
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
            $<BUILD_INTERFACE:${MUSAC_PROJECT_ROOT}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}
)

