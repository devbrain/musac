
add_subdirectory(sdk)
add_subdirectory(codecs)

if (MUSAC_BUILD_FPIC)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if (MUSAC_BUILD_SHARED)
    set(LIB_BUILD_TYPE SHARED)
else()
    set(LIB_BUILD_TYPE STATIC)
endif()

# Define source files
set(MUSAC_SOURCES
        audio_device.cc
        ${MUSAC_PROJECT_ROOT}/musac/audio_device.hh
        audio_system.cc
        ${MUSAC_PROJECT_ROOT}/musac/audio_system.hh
        audio_source.cc
        ${MUSAC_PROJECT_ROOT}/musac/audio_source.hh
        audio_loader.cc
        ${MUSAC_PROJECT_ROOT}/musac/audio_loader.hh



        lock.hh


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


        resampler/resampler_speex.cc
        resampler/resampler_speex.hh
        resampler/speex/arch.h
        resampler/speex/resample.c
        resampler/speex/resample_sse.h
        resampler/speex/speex_resampler.h

        ${codecs_sources}
        
        # Audio abstraction interfaces
        ${MUSAC_PROJECT_ROOT}/musac/audio_backend.hh
        ${MUSAC_PROJECT_ROOT}/musac/audio_device_interface.hh
        ${MUSAC_PROJECT_ROOT}/musac/audio_stream_interface.hh
        audio_backend_factory.cc
)

add_library(musac ${LIB_BUILD_TYPE} ${MUSAC_SOURCES})

# Add SDL3 backend sources conditionally
if(MUSAC_USE_SDL3)
    target_sources(musac PRIVATE
        backends/sdl3/sdl3_audio_backend.cc
        backends/sdl3/sdl3_audio_backend.hh
        backends/sdl3/sdl3_device_manager.cc
        backends/sdl3/sdl3_device_manager.hh
        backends/sdl3/sdl3_audio_stream.cc
        backends/sdl3/sdl3_audio_stream.hh
        resampler/resampler_sdl.cc
        resampler/resampler_sdl.hh
    )
endif()

target_link_libraries(musac PUBLIC musac_codecs musac_sdk thirdparty::failsafe)

# Set logger category for musac
target_compile_definitions(musac PRIVATE LOGGER_DEFAULT_CATEGORY=musac)

# Conditionally link SDL3
if(MUSAC_USE_SDL3)
    target_link_libraries(musac PRIVATE SDL3::SDL3)
    target_compile_definitions(musac PRIVATE MUSAC_HAS_SDL3_BACKEND)
endif()
target_include_directories(musac
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
            $<BUILD_INTERFACE:${MUSAC_PROJECT_ROOT}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MUSAC_BUILD_SHARED)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

    include(GenerateExportHeader)
    set(export_file_name "export_musac.h")

    generate_export_header(musac 
        EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/musac/${export_file_name}
    )
endif()

# For testing internal classes when building shared library
if(MUSAC_BUILD_TESTS AND MUSAC_BUILD_SHARED)
    # Create a static library with internal symbols for testing
    add_library(musac_internal STATIC ${MUSAC_SOURCES})
    
    # Add SDL3 backend sources conditionally
    if(MUSAC_USE_SDL3)
        target_sources(musac_internal PRIVATE
            backends/sdl3/sdl3_audio_backend.cc
            backends/sdl3/sdl3_audio_backend.hh
            backends/sdl3/sdl3_device_manager.cc
            backends/sdl3/sdl3_device_manager.hh
            backends/sdl3/sdl3_audio_stream.cc
            backends/sdl3/sdl3_audio_stream.hh
            resampler/resampler_sdl.cc
            resampler/resampler_sdl.hh
        )
    endif()
    
    target_link_libraries(musac_internal PUBLIC musac_codecs musac_sdk thirdparty::failsafe)
    
    if(MUSAC_USE_SDL3)
        target_link_libraries(musac_internal PRIVATE SDL3::SDL3)
        target_compile_definitions(musac_internal PRIVATE MUSAC_HAS_SDL3_BACKEND)
    endif()
    
    target_include_directories(musac_internal
            PUBLIC 
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
                $<BUILD_INTERFACE:${MUSAC_PROJECT_ROOT}>
                $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
            PRIVATE 
                ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    target_compile_definitions(musac_internal PRIVATE MUSAC_INTERNAL_TESTING)
endif()

