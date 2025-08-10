#ifndef MUSAC_SDK_AUDIO_CONVERTER_V2_HH
#define MUSAC_SDK_AUDIO_CONVERTER_V2_HH

#include <musac/sdk/types.hh>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/buffer.hh>
#include <musac/sdk/export_musac_sdk.h>
#include <stdexcept>
#include <cstddef>

namespace musac {

class MUSAC_SDK_EXPORT audio_converter {
public:
    // Convert and return a buffer - throws on error
    static buffer<uint8_t> convert(const audio_spec& src_spec, 
                                 const uint8_t* src_data, 
                                 size_t src_len,
                                 const audio_spec& dst_spec);
    
    // In-place conversion when possible (same format, different endianness)
    static void convert_in_place(audio_spec& spec, 
                                 uint8_t* data, 
                                 size_t len,
                                 const audio_spec& dst_spec);
    
    // Check if conversion is needed
    static bool needs_conversion(const audio_spec& from, const audio_spec& to) {
        return from.format != to.format || 
               from.channels != to.channels || 
               from.freq != to.freq;
    }
    
    // Fast path detection
    static bool has_fast_path(const audio_spec& from, const audio_spec& to);
    
    // Estimate output size without converting
    static size_t estimate_output_size(const audio_spec& src_spec,
                                       size_t src_len,
                                       const audio_spec& dst_spec);
    
    // Convert into existing buffer - returns bytes written
    static size_t convert_into(const audio_spec& src_spec,
                               const uint8_t* src_data,
                               size_t src_len,
                               const audio_spec& dst_spec,
                               buffer<uint8_t>& dst_buffer);
    
    // Streaming conversion for large files
    class stream_converter {
    public:
        stream_converter(const audio_spec& from, const audio_spec& to);
        ~stream_converter();
        
        // Process chunks without allocating full buffer
        size_t process_chunk(const uint8_t* input, size_t input_len,
                           buffer<uint8_t>& output);
        
        // Flush any remaining samples
        size_t flush(buffer<uint8_t>& output);
        
        // Reset the converter state
        void reset();
        
    private:
        struct impl;
        std::unique_ptr<impl> m_pimpl;
    };
};

// Exception types for audio conversion errors
class audio_conversion_error : public std::runtime_error {
public:
    explicit audio_conversion_error(const std::string& msg) 
        : std::runtime_error("Audio conversion error: " + msg) {}
};

class unsupported_format_error : public audio_conversion_error {
public:
    explicit unsupported_format_error(audio_format fmt) 
        : audio_conversion_error("Unsupported audio format: " + std::to_string(static_cast<int>(fmt))) {}
};

} // namespace musac

#endif // MUSAC_SDK_AUDIO_CONVERTER_V2_HH