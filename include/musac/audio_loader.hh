//
// Created by igor on 3/24/25.
//

#ifndef  AUDIO_LOADER_HH
#define  AUDIO_LOADER_HH

#include <filesystem>
#include <SDL3/SDL.h>
#include <musac/audio_source.hh>
#include <musac/sdk/resampler.hh>

#if !defined(d_MUSAC_LOAD_DECLARE)

#define d_MUSAC_LOAD_DECLARE(TYPE)                                                                                                  \
    audio_source load_ ## TYPE (SDL_IOStream* stream, bool do_close=false);                                                         \
    audio_source load_ ## TYPE (const std::filesystem::path& path);                                                                 \
    audio_source load_ ## TYPE (SDL_IOStream* stream, std::unique_ptr<resampler>&& resampler_obj, bool do_close=false);             \
    audio_source load_ ## TYPE (const std::filesystem::path& path, std::unique_ptr<resampler>&& resampler_obj)

#endif

namespace musac {

    template<typename Decoder>
    audio_source load_audio_source(SDL_IOStream* stream, bool do_close=false) {
        return {std::make_unique<Decoder>(), stream, do_close};
    }

    template<typename Decoder>
    audio_source load_audio_source(const std::filesystem::path& path) {
        return load_audio_source<Decoder>(SDL_IOFromFile(path.u8string().c_str(), "rb"), true);
    }

    template<typename Decoder>
    audio_source load_audio_source(SDL_IOStream* stream, std::unique_ptr<resampler>&& resampler_obj, bool do_close=false) {
        return {std::make_unique<Decoder>(), std::move(resampler_obj), stream, do_close};
    }

    template<typename Decoder>
    audio_source load_audio_source(const std::filesystem::path& path, std::unique_ptr<resampler>&& resampler_obj) {
        return load_audio_source<Decoder>(SDL_IOFromFile(path.u8string().c_str(), "rb"), std::move(resampler_obj), true);
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

}



#endif
