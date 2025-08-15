/**
 * @file decoders_registry.hh
 * @brief Audio decoder registry and format detection
 * @ingroup sdk_decoders
 */

#pragma once

#include <musac/sdk/export_musac_sdk.h>
#include <musac/sdk/io_stream.hh>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace musac {
    
    class decoder;
    
    /**
     * @class decoders_registry
     * @brief Registry for audio format decoders with automatic detection
     * @ingroup sdk_decoders
     * 
     * The decoders_registry manages available audio decoders and provides
     * automatic format detection. It maintains a prioritized list of decoders
     * and can automatically select the appropriate decoder for a given stream.
     * 
     * ## Architecture
     * 
     * The registry uses a two-phase approach:
     * 1. **Detection**: Each decoder's accept() function checks the stream
     * 2. **Creation**: If accepted, the factory creates a decoder instance
     * 
     * ## Usage Example
     * 
     * @code
     * // Create and populate registry
     * auto registry = std::make_shared<decoders_registry>();
     * 
     * // Register decoders with lambdas
     * registry->register_decoder(
     *     [](io_stream* s) { return decoder_mp3::accept(s); },
     *     []() { return std::make_unique<decoder_mp3>(); },
     *     10  // Higher priority for common formats
     * );
     * 
     * // Register using template helper (if available)
     * registry->register_decoder<decoder_flac>(5);
     * registry->register_decoder<decoder_wav>(10);
     * registry->register_decoder<decoder_vorbis>(8);
     * 
     * // Auto-detect and create decoder
     * auto stream = io_from_file("music.mp3", "rb");
     * auto decoder = registry->find_decoder(stream.get());
     * if (decoder) {
     *     decoder->open(stream.get());
     *     // Use decoder...
     * }
     * @endcode
     * 
     * ## Priority System
     * 
     * Decoders are checked in priority order (highest first):
     * - Priority 10+: Common formats (WAV, MP3, FLAC)
     * - Priority 5-9: Less common formats (OGG, MOD)
     * - Priority 0-4: Rare/legacy formats (VOC, CMF)
     * - Negative: Fallback decoders
     * 
     * ## Thread Safety
     * 
     * - Registration methods are NOT thread-safe
     * - find_decoder() and can_decode() ARE thread-safe for reading
     * - Typically configured once at startup
     * 
     * @see decoder, audio_source, audio_system
     */
    class MUSAC_SDK_EXPORT decoders_registry {
    public:
        /**
         * @typedef accept_func_t
         * @brief Function type for format detection
         * 
         * Returns true if the decoder can handle the stream format.
         */
        using accept_func_t = std::function<bool(io_stream*)>;
        
        /**
         * @typedef factory_func_t
         * @brief Function type for decoder creation
         * 
         * Creates a new instance of the decoder.
         */
        using factory_func_t = std::function<std::unique_ptr<decoder>()>;
        
        /**
         * @brief Register a decoder with the registry
         * @param accept Function to check if decoder handles the format
         * @param factory Function to create decoder instance
         * @param priority Priority level (higher = checked first, default 0)
         * 
         * Adds a decoder to the registry with specified priority.
         * Higher priority decoders are checked first during format detection.
         * 
         * @code
         * registry->register_decoder(
         *     [](io_stream* s) { 
         *         uint32_t magic;
         *         if (read_u32be(s, &magic)) {
         *             s->seek(-4, seek_origin::cur);
         *             return magic == 0x52494646;  // "RIFF"
         *         }
         *         return false;
         *     },
         *     []() { return std::make_unique<decoder_wav>(); },
         *     10  // High priority for common format
         * );
         * @endcode
         * 
         * @note Not thread-safe - configure at startup
         */
        void register_decoder(accept_func_t accept,
                            factory_func_t factory,
                            int priority = 0);
        
        /**
         * @brief Find decoder for the given stream
         * @param stream Input stream to analyze
         * @return Decoder instance if format recognized, nullptr otherwise
         * 
         * Checks each registered decoder in priority order until one
         * accepts the stream format. The stream position is preserved.
         * 
         * @code
         * auto decoder = registry->find_decoder(stream.get());
         * if (decoder) {
         *     decoder->open(stream.get());
         *     // Decoder ready to use
         * } else {
         *     // Format not supported
         * }
         * @endcode
         * 
         * @note Thread-safe for concurrent calls
         */
        [[nodiscard]] std::unique_ptr<decoder> find_decoder(io_stream* stream) const;
        
        /**
         * @brief Check if stream format is supported
         * @param stream Input stream to check
         * @return true if any decoder can handle the format
         * 
         * Quick check without creating a decoder instance.
         * Useful for validating files before processing.
         * 
         * @note Thread-safe for concurrent calls
         */
        [[nodiscard]] bool can_decode(io_stream* stream) const;
        
        /**
         * @brief Get number of registered decoders
         * @return Count of registered decoders
         */
        [[nodiscard]] size_t size() const;
        
        /**
         * @brief Remove all registered decoders
         * 
         * Clears the registry. Useful for testing or reconfiguration.
         * 
         * @note Not thread-safe - don't call while find_decoder() is running
         */
        void clear();
        
    private:
        struct decoder_entry {
            accept_func_t accept;
            factory_func_t factory;
            int priority;
        };
        
        std::vector<decoder_entry> m_decoders;
    };
    
} // namespace musac