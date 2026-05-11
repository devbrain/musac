#include <musac/codecs/register_codecs.hh>
#include <musac/sdk/decoders_registry.hh>

// Each decoder block is gated by MUSAC_HAS_<NAME>, defined per-axis by
// codecs/CMakeLists.txt. Disabled codecs are completely absent from
// the build — no header included, no decoder_<name>.cc compiled, no
// register_decoder() call here.

#if defined(MUSAC_HAS_DRWAV)
#include <musac/codecs/decoder_drwav.hh>
#endif
#if defined(MUSAC_HAS_DRMP3)
#include <musac/codecs/decoder_drmp3.hh>
#endif
#if defined(MUSAC_HAS_DRFLAC)
#include <musac/codecs/decoder_drflac.hh>
#endif
#if defined(MUSAC_HAS_VORBIS)
#include <musac/codecs/decoder_vorbis.hh>
#endif
#if defined(MUSAC_HAS_AIFF)
#include <musac/codecs/decoder_aiff.hh>
#endif
#if defined(MUSAC_HAS_8SVX)
#include <musac/codecs/decoder_8svx.hh>
#endif
#if defined(MUSAC_HAS_MODPLUG)
#include <musac/codecs/decoder_modplug.hh>
#endif
#if defined(MUSAC_HAS_VOC)
#include <musac/codecs/decoder_voc.hh>
#endif
#if defined(MUSAC_HAS_SEQ)
#include <musac/codecs/decoder_seq.hh>
#endif
#if defined(MUSAC_HAS_CMF)
#include <musac/codecs/decoder_cmf.hh>
#endif
#if defined(MUSAC_HAS_OPB)
#include <musac/codecs/decoder_opb.hh>
#endif
#if defined(MUSAC_HAS_VGM)
#include <musac/codecs/decoder_vgm.hh>
#endif
#if defined(MUSAC_HAS_MML)
#include <musac/codecs/decoder_mml.hh>
#endif

namespace musac {

void register_all_codecs(decoders_registry& registry) {
    // Priorities preserved from the original ordering — higher number
    // wins when multiple decoders accept the same byte pattern.
#if defined(MUSAC_HAS_DRWAV)
    registry.register_decoder(
        decoder_drwav::accept,
        []() { return std::make_unique<decoder_drwav>(); },
        100
    );
#endif
#if defined(MUSAC_HAS_DRMP3)
    registry.register_decoder(
        decoder_drmp3::accept,
        []() { return std::make_unique<decoder_drmp3>(); },
        90
    );
#endif
#if defined(MUSAC_HAS_DRFLAC)
    registry.register_decoder(
        decoder_drflac::accept,
        []() { return std::make_unique<decoder_drflac>(); },
        80
    );
#endif
#if defined(MUSAC_HAS_VORBIS)
    registry.register_decoder(
        decoder_vorbis::accept,
        []() { return std::make_unique<decoder_vorbis>(); },
        70
    );
#endif
#if defined(MUSAC_HAS_AIFF)
    registry.register_decoder(
        decoder_aiff::accept,
        []() { return std::make_unique<decoder_aiff>(); },
        60
    );
#endif
#if defined(MUSAC_HAS_MODPLUG)
    registry.register_decoder(
        decoder_modplug::accept,
        []() { return std::make_unique<decoder_modplug>(); },
        50
    );
#endif
#if defined(MUSAC_HAS_SEQ)
    registry.register_decoder(
        decoder_seq::accept,
        []() { return std::make_unique<decoder_seq>(); },
        40
    );
#endif
#if defined(MUSAC_HAS_VGM)
    registry.register_decoder(
        decoder_vgm::accept,
        []() { return std::make_unique<decoder_vgm>(); },
        30
    );
#endif
#if defined(MUSAC_HAS_VOC)
    registry.register_decoder(
        decoder_voc::accept,
        []() { return std::make_unique<decoder_voc>(); },
        25
    );
#endif
#if defined(MUSAC_HAS_8SVX)
    registry.register_decoder(
        decoder_8svx::accept,
        []() { return std::make_unique<decoder_8svx>(); },
        23
    );
#endif
#if defined(MUSAC_HAS_CMF)
    registry.register_decoder(
        decoder_cmf::accept,
        []() { return std::make_unique<decoder_cmf>(); },
        20
    );
#endif
#if defined(MUSAC_HAS_OPB)
    registry.register_decoder(
        decoder_opb::accept,
        []() { return std::make_unique<decoder_opb>(); },
        15
    );
#endif
#if defined(MUSAC_HAS_MML)
    registry.register_decoder(
        decoder_mml::accept,
        []() { return std::make_unique<decoder_mml>(); },
        10
    );
#endif
    // No-op when all codecs are disabled — registry stays empty.
    (void)registry;
}

std::shared_ptr<decoders_registry> create_registry_with_all_codecs() {
    auto registry = std::make_shared<decoders_registry>();
    register_all_codecs(*registry);
    return registry;
}

} // namespace musac