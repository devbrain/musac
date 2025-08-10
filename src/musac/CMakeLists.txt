
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
        ${MUSAC_PROJECT_ROOT}/include/musac/audio_device.hh
        audio_system.cc
        ${MUSAC_PROJECT_ROOT}/include/musac/audio_system.hh
        audio_source.cc
        ${MUSAC_PROJECT_ROOT}/include/musac/audio_source.hh


        stream.cc
        ${MUSAC_PROJECT_ROOT}/include/musac/stream.hh
        
        pc_speaker_stream.cc
        ${MUSAC_PROJECT_ROOT}/include/musac/pc_speaker_stream.hh
        pc_speaker_decoder.cc
        pc_speaker_decoder.hh

        fade_envelop.cc
        fade_envelop.hh
        in_use_guard.cc
        in_use_guard.hh
        callback_dispatcher.cc
        callback_dispatcher.hh
        audio_mixer.cc
        audio_mixer.hh
        ${MUSAC_PROJECT_ROOT}/include/musac/audio_device_data.hh



)

add_library(musac ${LIB_BUILD_TYPE} ${MUSAC_SOURCES})


target_link_libraries(musac PUBLIC musac_sdk thirdparty::failsafe)
target_compile_definitions(musac PRIVATE LOGGER_DEFAULT_CATEGORY=musac)


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

