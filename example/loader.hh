//
// Created by igor on 3/24/25.
//

#ifndef  LOADER_HH
#define  LOADER_HH

#include <musac/audio_source.hh>

enum class music_type {
    cmf,
    hmp,
    mid,
    mp3,
    opb,
    s3m,
    voc,
    xmi
};

struct loader {
    static void init();
    static void done();

    static musac::audio_source load(music_type type);
};



#endif
