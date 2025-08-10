#ifndef MUSAC_CODECS_REGISTER_CODECS_HH
#define MUSAC_CODECS_REGISTER_CODECS_HH

#include <musac/codecs/export_musac_codecs.h>
#include <memory>

namespace musac {
    class decoders_registry;
    
    /**
     * Register all available codecs with the provided registry.
     * 
     * This function registers all decoders from the musac_codecs library
     * with the given registry. The decoders are registered with appropriate
     * priorities to ensure optimal format detection.
     * 
     * @param registry The registry to register decoders with
     */
    MUSAC_CODECS_EXPORT void register_all_codecs(decoders_registry& registry);
    
    /**
     * Create a new registry with all codecs pre-registered.
     * 
     * This is a convenience function that creates a new registry
     * and registers all available codecs with it.
     * 
     * @return A shared pointer to a registry with all codecs registered
     */
    MUSAC_CODECS_EXPORT std::shared_ptr<decoders_registry> create_registry_with_all_codecs();
}

#endif // MUSAC_CODECS_REGISTER_CODECS_HH