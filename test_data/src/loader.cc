//
// Created by igor on 3/24/25.
//

#include <vector>
#include <tuple>
#include <array>
#include <magic_enum/magic_enum.hpp>

#include <musac/test_data/loader.hh>

#include "../data/cmf_example.h"
#include "../data/hmp_example.h"
#include "../data/mid_example.h"
#include "../data/mml_bouree.h"
#include "../data/mml_complex.h"
#include "../data/mp3_example.h"
#include "../data/opb_example.h"
#include "../data/s3m_example.h"
#include "../data/voc_example.h"
#include "../data/xmi_example.h"

#include <musac/audio_loader.hh>


namespace musac::test_data {

using loader_func_t = musac::audio_source (*)(std::unique_ptr<musac::io_stream>);
using data_t = std::tuple<music_type, const unsigned char*, std::size_t, loader_func_t>;

#define S(TYPE) data_t{music_type::TYPE, TYPE ## _example_ ## TYPE, sizeof(TYPE ## _example_ ## TYPE), musac::load_ ## TYPE}
#define S2(TYPE, FN) data_t{music_type::TYPE, TYPE ## _example_ ## TYPE, sizeof(TYPE ## _example_ ## TYPE), FN}
#define S3(TYPE, VAR, FN) data_t{music_type::TYPE, VAR ## _mml, sizeof(VAR ## _mml), FN}

static constexpr auto mus_count = magic_enum::enum_count<music_type>();
static std::array<data_t, mus_count> s_data = {
    S(cmf),
    S2(hmp, musac::load_midi),
    S2(mid, musac::load_midi),
    S3(mml_bouree, mml_bouree, musac::load_mml),
    S3(mml_complex, mml_complex, musac::load_mml),
    S(mp3),
    S(opb),
    S2(s3m, musac::load_mod),
    S(voc),
    S2(xmi, musac::load_midi)
};

#include <musac/sdk/io_stream.hh>

void loader::init() {
    // No longer need to pre-create streams
}

void loader::done() {
    // No longer need to clean up streams
}

musac::audio_source loader::load(music_type type) {
    auto idx = static_cast <unsigned int>(type);
    auto ldr = std::get<3>(s_data[idx]);
    
    // We need to create a new io_stream for each load since ownership is transferred
    const auto& [t, buf, sz, fn] = s_data[idx];
    return ldr(musac::io_from_memory(buf, sz));
}

} // namespace musac::test_data
