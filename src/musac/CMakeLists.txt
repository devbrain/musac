# MUSAC Main Library
cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Subdirectories (must be included first)
# ============================================================================

add_subdirectory(sdk)
add_subdirectory(codecs)

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
# Sources
# ============================================================================

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

    # Resampler (private implementation)
    resampler/resampler_speex.cc
    resampler/resampler_speex.hh
    resampler/speex/resample.c
)

# ============================================================================
# Create Library
# ============================================================================

add_library(musac ${LIB_BUILD_TYPE} ${MUSAC_SOURCES})
add_library(musac::musac ALIAS musac)

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

# ============================================================================
# Export Header
# ============================================================================

include(GenerateExportHeader)
set(export_file_name "export_musac.h")

generate_export_header(musac
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/musac/${export_file_name}
    STATIC_DEFINE MUSAC_STATIC_DEFINE
)

if(MUSAC_BUILD_SHARED)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
else()
    target_compile_definitions(musac PUBLIC MUSAC_STATIC_DEFINE)
endif()

# Compiler Warnings
neutrino_target_warnings(musac)

# Suppress warnings for third-party code (speex resampler)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(
        resampler/speex/resample.c
        PROPERTIES COMPILE_OPTIONS "-w"
    )
elseif(MSVC)
    set_source_files_properties(
        resampler/speex/resample.c
        PROPERTIES COMPILE_FLAGS "/W0"
    )
endif()

# ============================================================================
# Installation (handled in root CMakeLists.txt via NeutrinoInstall)
# ============================================================================

if(NOT MUSAC_IS_SUBDIRECTORY AND NEUTRINO_MUSAC_INSTALL)
    # Install generated export header if shared build
    if(MUSAC_BUILD_SHARED)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/musac/export_musac.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/musac
        )
    endif()
endif()
