set (sdk_sources
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/samples_converter.hh
        samples_converter.cc

        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/buffer.hh
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/decoder.hh
        decoder.cc
        
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/decoders_registry.hh
        decoders_registry.cc

        processor.cc
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/processor.hh

        resampler.cc
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/resampler.hh
        from_float_converter.cc
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/from_float_converter.hh
        
        # New core headers and sources
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/types.hh
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/audio_format.hh
        audio_format.cc

        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/io_stream.hh
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/endian.hh
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/audio_converter.hh

        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/audio_backend.hh
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/audio_stream_interface.hh
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/compiler.hh

        ${CMAKE_CURRENT_BINARY_DIR}/musac/sdk/musac_sdk_config.h
        io_stream.cc
        audio_converter.cc
        audio_converter_internal.cc
        
        # MML parser utility
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/mml_parser.hh
        mml_parser.cc

        # Resampler
        resampler/resampler_speex.cc
        resampler/resampler_speex.hh
        resampler/speex/arch.h
        resampler/speex/resample.c
        resampler/speex/resample_sse.h
        resampler/speex/speex_resampler.h
)

set(opl_sources
        opl/adlib_emu.cpp
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/opl/adlib_emu.h
        opl/impl_opl.c
        opl/impl_opl.h
        opl/opl.cc
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/opl/opl.hh
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/opl/opl_command.h
        opl/opl_player.cc
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/opl/opl_player.hh
        # New chip emulator interface
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/opl/chip_emulator.hh
        opl/chip_emulator_impl.cc
        # Legacy compatibility header
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/opl/ymfm_chip.hh
        # Internal YMFM sources (no longer in public headers)
        opl/ymfm/ymfm_adpcm.cpp
        opl/ymfm/ymfm_misc.cpp
        opl/ymfm/ymfm_opl.cpp
        opl/ymfm/ymfm_opm.cpp
        opl/ymfm/ymfm_opn.cpp
        opl/ymfm/ymfm_opq.cpp
        opl/ymfm/ymfm_opz.cpp
        opl/ymfm/ymfm_pcm.cpp
        opl/ymfm/ymfm_ssg.cpp
)

set(midi_sources
        # Public headers
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/midi/opl_midi_synth.hh
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/midi/opl_patches.hh
        ${MUSAC_PROJECT_ROOT}/include/musac/sdk/midi/midi_sequence.hh
        # Implementation files
        midi/opl_midi_synth.cc
        midi/opl_midi_synth_impl.cc
        midi/opl_midi_synth_impl.hh
        midi/opl_patches.cc
        midi/patchnames.cc
        midi/midi_sequence.cc
        # Sequence format implementations
        midi/sequence.h
        midi/sequence.cpp
        midi/sequence_mid.h
        midi/sequence_mid.cpp
        midi/sequence_mus.h
        midi/sequence_mus.cpp
        midi/sequence_xmi.h
        midi/sequence_xmi.cpp
        midi/sequence_hmi.h
        midi/sequence_hmi.cpp
        midi/sequence_hmp.h
        midi/sequence_hmp.cpp
        # Patch data
        midi/midi_opl_data.cc
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
        ${midi_sources}
)

target_include_directories(musac_sdk
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
            $<BUILD_INTERFACE:${MUSAC_PROJECT_ROOT}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/opl  # For internal YMFM headers
            ${CMAKE_CURRENT_SOURCE_DIR}/midi  # For internal MIDI headers
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