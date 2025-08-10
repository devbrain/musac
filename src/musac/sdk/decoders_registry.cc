#include <musac/sdk/decoders_registry.hh>
#include <musac/sdk/decoder.hh>
#include <algorithm>

namespace musac {

void decoders_registry::register_decoder(accept_func_t accept,
                                        factory_func_t factory,
                                        int priority) {
    m_decoders.push_back({accept, factory, priority});
    
    // Sort by priority (higher first)
    std::stable_sort(m_decoders.begin(), m_decoders.end(),
                    [](const decoder_entry& a, const decoder_entry& b) {
                        return a.priority > b.priority;
                    });
}

std::unique_ptr<decoder> decoders_registry::find_decoder(io_stream* stream) const {
    if (!stream) {
        return nullptr;
    }
    
    // Save current stream position
    auto original_pos = stream->tell();
    if (original_pos < 0) {
        return nullptr;
    }
    
    for (const auto& entry : m_decoders) {
        // Reset stream position before each check
        stream->seek(original_pos, seek_origin::set);
        
        try {
            if (entry.accept && entry.accept(stream)) {
                // Restore position before creating decoder
                stream->seek(original_pos, seek_origin::set);
                
                if (entry.factory) {
                    return entry.factory();
                }
            }
        } catch (...) {
            // If accept throws, continue to next decoder
        }
    }
    
    // Restore original position if no decoder found
    stream->seek(original_pos, seek_origin::set);
    return nullptr;
}

bool decoders_registry::can_decode(io_stream* stream) const {
    if (!stream) {
        return false;
    }
    
    // Save current stream position
    auto original_pos = stream->tell();
    if (original_pos < 0) {
        return false;
    }
    
    for (const auto& entry : m_decoders) {
        // Reset stream position before each check
        stream->seek(original_pos, seek_origin::set);
        
        try {
            if (entry.accept && entry.accept(stream)) {
                // Restore position
                stream->seek(original_pos, seek_origin::set);
                return true;
            }
        } catch (...) {
            // If accept throws, continue to next decoder
        }
    }
    
    // Restore original position
    stream->seek(original_pos, seek_origin::set);
    return false;
}

size_t decoders_registry::size() const {
    return m_decoders.size();
}

void decoders_registry::clear() {
    m_decoders.clear();
}

} // namespace musac