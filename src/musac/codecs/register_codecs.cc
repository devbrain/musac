#include <musac/codecs/register_codecs.hh>
#include <musac/sdk/decoders_registry.hh>

// Include all decoder headers
#include <musac/codecs/decoder_drwav.hh>
#include <musac/codecs/decoder_drmp3.hh>
#include <musac/codecs/decoder_drflac.hh>
#include <musac/codecs/decoder_vorbis.hh>
#include <musac/codecs/decoder_aiff.hh>
#include <musac/codecs/decoder_8svx.hh>
#include <musac/codecs/decoder_modplug.hh>
#include <musac/codecs/decoder_voc.hh>
#include <musac/codecs/decoder_seq.hh>
#include <musac/codecs/decoder_cmf.hh>
#include <musac/codecs/decoder_opb.hh>
#include <musac/codecs/decoder_vgm.hh>
#include <musac/codecs/decoder_mml.hh>

namespace musac {

void register_all_codecs(decoders_registry& registry) {
    // Register common audio formats with higher priority
    // These are likely to be encountered more frequently
    
    // WAV - very common format, highest priority
    registry.register_decoder(
        decoder_drwav::accept,
        []() { return std::make_unique<decoder_drwav>(); },
        100
    );
    
    // MP3 - very common format
    registry.register_decoder(
        decoder_drmp3::accept,
        []() { return std::make_unique<decoder_drmp3>(); },
        90
    );
    
    // FLAC - common lossless format
    registry.register_decoder(
        decoder_drflac::accept,
        []() { return std::make_unique<decoder_drflac>(); },
        80
    );
    
    // Vorbis/OGG - common open format
    registry.register_decoder(
        decoder_vorbis::accept,
        []() { return std::make_unique<decoder_vorbis>(); },
        70
    );
    
    // AIFF - common on Mac
    registry.register_decoder(
        decoder_aiff::accept,
        []() { return std::make_unique<decoder_aiff>(); },
        60
    );
    
    // Module formats (MOD/S3M/XM/IT) - used in games and demos
    registry.register_decoder(
        decoder_modplug::accept,
        []() { return std::make_unique<decoder_modplug>(); },
        50
    );
    
    // MIDI-like sequence formats
    registry.register_decoder(
        decoder_seq::accept,
        []() { return std::make_unique<decoder_seq>(); },
        40
    );
    
    // Specialized/retro formats with lower priority
    
    // VGM - Video Game Music format
    registry.register_decoder(
        decoder_vgm::accept,
        []() { return std::make_unique<decoder_vgm>(); },
        30
    );
    
    // VOC - Creative Voice File
    registry.register_decoder(
        decoder_voc::accept,
        []() { return std::make_unique<decoder_voc>(); },
        25
    );
    
    // 8SVX - Amiga 8-bit Sampled Voice
    registry.register_decoder(
        decoder_8svx::accept,
        []() { return std::make_unique<decoder_8svx>(); },
        23
    );
    
    // CMF - Creative Music File
    registry.register_decoder(
        decoder_cmf::accept,
        []() { return std::make_unique<decoder_cmf>(); },
        20
    );
    
    // OPB - OPL Binary format
    registry.register_decoder(
        decoder_opb::accept,
        []() { return std::make_unique<decoder_opb>(); },
        15
    );
    
    // MML - Music Macro Language (text-based, lowest priority)
    registry.register_decoder(
        decoder_mml::accept,
        []() { return std::make_unique<decoder_mml>(); },
        10
    );
}

std::shared_ptr<decoders_registry> create_registry_with_all_codecs() {
    auto registry = std::make_shared<decoders_registry>();
    register_all_codecs(*registry);
    return registry;
}

} // namespace musac