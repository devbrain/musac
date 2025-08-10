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
#include "../data/ogg_example.h"

#include <musac/audio_source.hh>
#include <musac/codecs/register_codecs.hh>
#include <musac/sdk/io_stream.hh>

namespace musac::test_data {

using data_t = std::tuple<music_type, const unsigned char*, std::size_t>;

static constexpr auto mus_count = magic_enum::enum_count<music_type>();
static std::array<data_t, mus_count> s_data = {
    data_t{music_type::cmf, cmf_example_cmf, sizeof(cmf_example_cmf)},
    data_t{music_type::hmp, hmp_example_hmp, sizeof(hmp_example_hmp)},
    data_t{music_type::mid, mid_example_mid, sizeof(mid_example_mid)},
    data_t{music_type::mml_bouree, mml_bouree_mml, sizeof(mml_bouree_mml)},
    data_t{music_type::mml_complex, mml_complex_mml, sizeof(mml_complex_mml)},
    data_t{music_type::mp3, mp3_example_mp3, sizeof(mp3_example_mp3)},
    data_t{music_type::opb, opb_example_opb, sizeof(opb_example_opb)},
    data_t{music_type::s3m, s3m_example_s3m, sizeof(s3m_example_s3m)},
    data_t{music_type::voc, voc_example_voc, sizeof(voc_example_voc)},
    data_t{music_type::xmi, xmi_example_xmi, sizeof(xmi_example_xmi)},
    data_t{music_type::vorbis, punch_ogg_input, sizeof(punch_ogg_input)}
};

// Static registry for decoders
static std::shared_ptr<musac::decoders_registry> s_registry;

void loader::init() {
    // Create registry with all codecs registered
    s_registry = musac::create_registry_with_all_codecs();
}

void loader::done() {
    // Clean up registry
    s_registry.reset();
}

musac::audio_source loader::load(music_type type) {
    auto idx = static_cast <unsigned int>(type);
    
    // Get the data for this music type
    const auto& [t, buf, sz] = s_data[idx];
    
    // Create io_stream from memory buffer
    auto stream = musac::io_from_memory(buf, sz);
    
    // Use the new audio_source constructor with automatic format detection
    return musac::audio_source(std::move(stream), s_registry.get());
}

} // namespace musac::test_data
