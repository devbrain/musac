//
// Created by igor on 3/24/25.
//

#include <musac/codecs/decoder_aiff.hh>
#include <musac/codecs/decoder_cmf.hh>
#include <musac/codecs/decoder_drflac.hh>
#include <musac/codecs/decoder_drmp3.hh>
#include <musac/codecs/decoder_drwav.hh>
#include <musac/codecs/decoder_modplug.hh>
#include <musac/codecs/decoder_opb.hh>
#include <musac/codecs/decoder_seq.hh>
#include <musac/codecs/decoder_vgm.hh>
#include <musac/codecs/decoder_voc.hh>


namespace musac {
    using decoder_flac = decoder_drflac;
    using decoder_mp3 = decoder_drmp3;
    using decoder_wav = decoder_drwav;
    using decoder_mod = decoder_modplug;
    using decoder_midi = decoder_seq;


}

#define d_MUSAC_LOAD_DECLARE(TYPE)                                                                                     \
audio_source load_ ## TYPE (SDL_IOStream* stream, bool do_close) {                                                     \
    return load_audio_source<decoder_ ## TYPE>(stream, do_close);                                                      \
}                                                                                                                      \
audio_source load_ ## TYPE (const std::filesystem::path& path) {                                                       \
    return load_audio_source<decoder_ ## TYPE>(path);                                                                  \
}                                                                                                                      \
audio_source load_ ## TYPE (SDL_IOStream* stream, std::unique_ptr<resampler>&& resampler_obj, bool do_close)  {        \
    return load_audio_source<decoder_ ## TYPE>(stream, std::move(resampler_obj), do_close);                            \
}                                                                                                                      \
audio_source load_ ## TYPE (const std::filesystem::path& path, std::unique_ptr<resampler>&& resampler_obj) {           \
    return load_audio_source<decoder_ ## TYPE>(path, std::move(resampler_obj));                                        \
}

#include "../../include/musac/audio_loader.hh"
