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
     * Registry for audio decoders.
     * Allows registration of decoder factories with format detection.
     */
    class MUSAC_SDK_EXPORT decoders_registry {
    public:
        using accept_func_t = std::function<bool(io_stream*)>;
        using factory_func_t = std::function<std::unique_ptr<decoder>()>;
        
        /**
         * Register a decoder with its accept function and factory.
         * @param accept Function to check if decoder can handle the format
         * @param factory Function to create decoder instance
         * @param priority Optional priority (higher = checked first, default 0)
         */
        void register_decoder(accept_func_t accept,
                            factory_func_t factory,
                            int priority = 0);
        
        /**
         * Find a decoder that can handle the given stream.
         * @param stream The input stream to check
         * @return Decoder instance if found, nullptr otherwise
         */
        [[nodiscard]] std::unique_ptr<decoder> find_decoder(io_stream* stream) const;
        
        /**
         * Check if any registered decoder can handle the stream.
         * @param stream The input stream to check
         * @return true if a compatible decoder is found
         */
        [[nodiscard]] bool can_decode(io_stream* stream) const;
        
        /**
         * Get the number of registered decoders.
         */
        [[nodiscard]] size_t size() const;
        
        /**
         * Clear all registered decoders.
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