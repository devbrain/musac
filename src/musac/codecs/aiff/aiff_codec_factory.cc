#include <musac/codecs/aiff/aiff_codec_factory.hh>
#include <musac/error.hh>
#include <stdexcept>

// Forward declare factory functions from codec implementations
namespace musac {
namespace aiff {
    std::unique_ptr<aiff_codec_base> create_pcm_codec();
    std::unique_ptr<aiff_codec_base> create_ima4_codec();
    std::unique_ptr<aiff_codec_base> create_float_codec();
    std::unique_ptr<aiff_codec_base> create_ulaw_codec();
    std::unique_ptr<aiff_codec_base> create_alaw_codec();
}
}

namespace musac {
namespace aiff {

// Static initialization flag
bool aiff_codec_factory::s_initialized = false;

// Compression types (matching those in codec files)
static const iff::fourcc COMP_NONE("NONE");
static const iff::fourcc COMP_SOWT("sowt");
static const iff::fourcc COMP_FL32("fl32");
static const iff::fourcc COMP_FL64("fl64");
static const iff::fourcc COMP_ALAW("ALAW");
static const iff::fourcc COMP_ULAW("ULAW");
static const iff::fourcc COMP_alaw("alaw");
static const iff::fourcc COMP_ulaw("ulaw");
static const iff::fourcc COMP_IMA4("ima4");

std::map<iff::fourcc, aiff_codec_factory::codec_creator>& 
aiff_codec_factory::get_registry() {
    static std::map<iff::fourcc, codec_creator> registry;
    return registry;
}

void aiff_codec_factory::initialize_default_codecs() {
    if (s_initialized) {
        return;
    }
    
    auto& registry = get_registry();
    
    // Register PCM codecs
    registry[COMP_NONE] = []() { 
        return create_pcm_codec();
    };
    
    registry[COMP_SOWT] = []() { 
        // SOWT handling will be done based on compression type in the codec
        return create_pcm_codec();
    };
    
    // Register float codecs
    registry[COMP_FL32] = []() { 
        return create_float_codec();
    };
    
    registry[COMP_FL64] = []() { 
        return create_float_codec();
    };
    
    // Register G.711 codecs (both uppercase and lowercase)
    registry[COMP_ULAW] = []() { return create_ulaw_codec(); };
    registry[COMP_ulaw] = []() { return create_ulaw_codec(); };
    registry[COMP_ALAW] = []() { return create_alaw_codec(); };
    registry[COMP_alaw] = []() { return create_alaw_codec(); };
    
    // Register IMA4 codec
    registry[COMP_IMA4] = []() { return create_ima4_codec(); };
    
    s_initialized = true;
}

std::unique_ptr<aiff_codec_base> aiff_codec_factory::create(
    const iff::fourcc& compression_type,
    const codec_params& params) {
    
    // Ensure default codecs are registered
    initialize_default_codecs();
    
    auto& registry = get_registry();
    auto it = registry.find(compression_type);
    
    if (it == registry.end()) {
        throw std::runtime_error("Unsupported AIFF compression type: " + compression_type.to_string());
    }
    
    // Create and initialize the codec
    auto codec = it->second();
    if (!codec) {
        throw std::runtime_error("Failed to create codec for compression type: " + compression_type.to_string());
    }
    
    // Initialize with parameters
    codec->initialize(params);
    
    // Special handling for specific codecs
    if (compression_type == COMP_SOWT) {
        // Need to use the pcm_codec class definition
        // This is handled in the factory lambda above
    }
    
    return codec;
}

void aiff_codec_factory::register_codec(
    const iff::fourcc& compression_type,
    codec_creator creator) {
    
    initialize_default_codecs();
    get_registry()[compression_type] = creator;
}

bool aiff_codec_factory::has_codec(const iff::fourcc& compression_type) {
    initialize_default_codecs();
    auto& registry = get_registry();
    return registry.find(compression_type) != registry.end();
}

} // namespace aiff
} // namespace musac