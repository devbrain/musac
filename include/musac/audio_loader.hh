//
// Created by igor on 3/24/25.
//

#ifndef  AUDIO_LOADER_HH
#define  AUDIO_LOADER_HH

#include <filesystem>
#include <musac/audio_source.hh>
#include <musac/sdk/io_stream.hh>
#include <musac/sdk/resampler.hh>
#include <musac/export_musac.h>

#if !defined(d_MUSAC_LOAD_DECLARE)

#define d_MUSAC_LOAD_DECLARE(TYPE)                                                                                                  \
    MUSAC_EXPORT audio_source load_ ## TYPE (std::unique_ptr<musac::io_stream> stream);                                            \
    MUSAC_EXPORT audio_source load_ ## TYPE (const std::filesystem::path& path);                                                    \
    MUSAC_EXPORT audio_source load_ ## TYPE (std::unique_ptr<musac::io_stream> stream, std::unique_ptr<resampler>&& resampler_obj);\
    MUSAC_EXPORT audio_source load_ ## TYPE (const std::filesystem::path& path, std::unique_ptr<resampler>&& resampler_obj)

#endif

namespace musac {

    template<typename Decoder>
    audio_source load_audio_source(std::unique_ptr<musac::io_stream> stream) {
        return {std::make_unique<Decoder>(), std::move(stream)};
    }

    template<typename Decoder>
    audio_source load_audio_source(const std::filesystem::path& path) {
        return load_audio_source<Decoder>(musac::io_from_file(path.u8string().c_str(), "rb"));
    }

    template<typename Decoder>
    audio_source load_audio_source(std::unique_ptr<musac::io_stream> stream, std::unique_ptr<resampler>&& resampler_obj) {
        return {std::make_unique<Decoder>(), std::move(resampler_obj), std::move(stream)};
    }

    template<typename Decoder>
    audio_source load_audio_source(const std::filesystem::path& path, std::unique_ptr<resampler>&& resampler_obj) {
        return load_audio_source<Decoder>(musac::io_from_file(path.u8string().c_str(), "rb"), std::move(resampler_obj));
    }

    d_MUSAC_LOAD_DECLARE(wav);
    d_MUSAC_LOAD_DECLARE(mp3);
    d_MUSAC_LOAD_DECLARE(flac);
    d_MUSAC_LOAD_DECLARE(voc);
    d_MUSAC_LOAD_DECLARE(aiff);

    d_MUSAC_LOAD_DECLARE(cmf);
    d_MUSAC_LOAD_DECLARE(mod);
    d_MUSAC_LOAD_DECLARE(midi);
    d_MUSAC_LOAD_DECLARE(opb);
    d_MUSAC_LOAD_DECLARE(vgm);
    d_MUSAC_LOAD_DECLARE(mml);

}



#endif
