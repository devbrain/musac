//
// Created by igor on 3/24/25.
//

#ifndef  MUSAC_TEST_DATA_LOADER_HH
#define  MUSAC_TEST_DATA_LOADER_HH

#include <musac/audio_source.hh>


namespace musac::test_data {

enum class music_type {
    cmf,
    hmp,
    mid,
    mml_bouree,
    mml_complex,
    mp3,
    opb,
    s3m,
    voc,
    xmi,
    vorbis
};

struct loader {
    static void init();
    static void done();

    static musac::audio_source load(music_type type);
    
    // Returns true if the music_type is a music file, false if it's a sound effect
    static bool is_music(music_type type);
    
    // Returns a human-readable name for the music_type
    static const char* get_name(music_type type);
};

} // namespace musac::test_data


#endif