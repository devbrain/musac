#pragma once

#include <musac/codecs/aiff/aiff_codec_base.hh>
#include <iff/fourcc.hh>
#include <memory>
#include <functional>
#include <map>

namespace musac {
namespace aiff {

// Factory for creating AIFF compression codecs
class aiff_codec_factory {
public:
    // Codec creator function type
    using codec_creator = std::function<std::unique_ptr<aiff_codec_base>()>;
    
    // Create a codec for the given compression type
    static std::unique_ptr<aiff_codec_base> create(
        const iff::fourcc& compression_type,
        const codec_params& params);
    
    // Register a custom codec
    static void register_codec(
        const iff::fourcc& compression_type,
        codec_creator creator);
    
    // Check if a codec is available for the compression type
    static bool has_codec(const iff::fourcc& compression_type);
    
private:
    // Get the codec registry
    static std::map<iff::fourcc, codec_creator>& get_registry();
    
    // Initialize default codecs
    static void initialize_default_codecs();
    
    // Flag to track initialization
    static bool s_initialized;
};

} // namespace aiff
} // namespace musac