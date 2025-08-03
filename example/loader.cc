//
// Created by igor on 3/24/25.
//

#include <vector>
#include <tuple>
#include <array>
#include <magic_enum/magic_enum.hpp>

#include "loader.hh"

#include "data/cmf_example.h"
#include "data/hmp_example.h"
#include "data/mid_example.h"
#include "data/mp3_example.h"
#include "data/opb_example.h"
#include "data/s3m_example.h"
#include "data/voc_example.h"
#include "data/xmi_example.h"

#include <musac/audio_loader.hh>

using loader_func_t = musac::audio_source (*)(musac::io_stream*, bool);
using data_t = std::tuple<music_type, const unsigned char*, std::size_t, loader_func_t>;

#define S(TYPE) data_t{music_type::TYPE, TYPE ## _example_ ## TYPE, TYPE ## _example_ ## TYPE ## _size, musac::load_ ## TYPE}
#define S2(TYPE, FN) data_t{music_type::TYPE, TYPE ## _example_ ## TYPE, TYPE ## _example_ ## TYPE ## _size, FN}

static constexpr auto mus_count = magic_enum::enum_count<music_type>();
static std::array<data_t, mus_count> s_data = {
    S(cmf),
    S2(hmp, musac::load_midi),
    S2(mid, musac::load_midi),
    S(mp3),
    S(opb),
    S2(s3m, musac::load_mod),
    S(voc),
    S2(xmi, musac::load_midi)
};

#include <musac/sdk/io_stream.h>

static std::vector<std::unique_ptr<musac::io_stream>> s_streams;

void loader::init() {
    s_streams.clear();
    s_streams.reserve(mus_count);
    for (size_t i = 0; i < mus_count; ++i) {
        s_streams.push_back(nullptr);
    }
    for (const auto& [t, buf, sz, fn] : s_data) {
        s_streams[static_cast <int>(t)] = musac::io_from_memory(buf, sz);
    }
}

void loader::done() {
    s_streams.clear();
}

musac::audio_source loader::load(music_type type) {
    if (s_streams.empty()) {
        init();
    }
    auto idx = static_cast <int>(type);
    auto ldr = std::get<3>(s_data[idx]);
    return ldr(s_streams[idx].get(), false);
}
