//
// Created by igor on 3/24/25.
//

#ifndef  MUSAC_TEST_DATA_LOADER_HH
#define  MUSAC_TEST_DATA_LOADER_HH

#include <musac/audio_source.hh>

namespace musac {
namespace test_data {

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
};

} // namespace test_data
} // namespace musac

#endif