/**
 * @file audio_converter.hh
 * @brief Audio format conversion utilities
 * @ingroup sdk_conversion
 */

#ifndef MUSAC_SDK_AUDIO_CONVERTER_V2_HH
#define MUSAC_SDK_AUDIO_CONVERTER_V2_HH

#include <musac/sdk/types.hh>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/buffer.hh>
#include <musac/sdk/export_musac_sdk.h>
#include <stdexcept>
#include <cstddef>

namespace musac {

/**
 * @class audio_converter
 * @brief High-performance audio format conversion
 * @ingroup sdk_conversion
 * 
 * The audio_converter provides comprehensive audio format conversion
 * including format changes, channel mixing, and sample rate conversion.
 * It automatically selects optimized paths for common conversions.
 * 
 * ## Supported Conversions
 * 
 * - **Format**: 8/16/32-bit integer, 32-bit float
 * - **Channels**: Mono↔Stereo, multi-channel downmix
 * - **Sample Rate**: High-quality resampling (via resampler)
 * - **Endianness**: Big↔Little endian
 * 
 * ## Usage Examples
 * 
 * @code
 * // Simple conversion
 * audio_spec src_spec{audio_format::s16le, 2, 44100};
 * audio_spec dst_spec{audio_format::f32le, 2, 48000};
 * 
 * auto converted = audio_converter::convert(
 *     src_spec, src_data, src_len, dst_spec
 * );
 * 
 * // Streaming conversion for large files
 * audio_converter::stream_converter conv(src_spec, dst_spec);
 * buffer<uint8_t> chunk_out(8192);
 * 
 * while (has_input()) {
 *     auto input = read_chunk();
 *     size_t written = conv.process_chunk(
 *         input.data(), input.size(), chunk_out
 *     );
 *     write_output(chunk_out.data(), written);
 * }
 * @endcode
 * 
 * ## Performance Characteristics
 * 
 * - Fast paths for common conversions (S16→F32, stereo→mono)
 * - SIMD optimization where available
 * - Zero-copy for endianness-only conversions
 * - Streaming API avoids large allocations
 * 
 * @see resampler, audio_spec, audio_format
 */
class MUSAC_SDK_EXPORT audio_converter {
public:
    /**
     * @brief Convert audio data to new format
     * @param src_spec Source audio specification
     * @param src_data Source audio data
     * @param src_len Source data length in bytes
     * @param dst_spec Destination audio specification
     * @return Buffer containing converted audio
     * @throws audio_conversion_error on conversion failure
     * 
     * Performs complete audio conversion including format, channels,
     * and sample rate. Automatically allocates output buffer.
     */
    static buffer<uint8_t> convert(const audio_spec& src_spec, 
                                 const uint8_t* src_data, 
                                 size_t src_len,
                                 const audio_spec& dst_spec);
    
    /**
     * @brief Convert audio in-place when possible
     * @param spec Audio specification (updated to dst_spec on success)
     * @param data Audio data buffer
     * @param len Data length in bytes
     * @param dst_spec Target specification
     * @throws audio_conversion_error if in-place conversion not possible
     * 
     * Optimized for endianness-only conversions. Modifies data in-place
     * without allocation when possible.
     */
    static void convert_in_place(audio_spec& spec, 
                                 uint8_t* data, 
                                 size_t len,
                                 const audio_spec& dst_spec);
    
    /**
     * @brief Check if conversion is needed
     * @param from Source specification
     * @param to Target specification
     * @return true if specifications differ
     */
    static bool needs_conversion(const audio_spec& from, const audio_spec& to) {
        return from.format != to.format || 
               from.channels != to.channels || 
               from.freq != to.freq;
    }
    
    /**
     * @brief Check if fast conversion path exists
     * @param from Source specification
     * @param to Target specification
     * @return true if optimized conversion available
     * 
     * Fast paths exist for common conversions like S16→F32,
     * stereo→mono, and endianness swapping.
     */
    static bool has_fast_path(const audio_spec& from, const audio_spec& to);
    
    /**
     * @brief Estimate output buffer size
     * @param src_spec Source specification
     * @param src_len Source length in bytes
     * @param dst_spec Target specification
     * @return Required output buffer size in bytes
     * 
     * Use before allocating buffers for convert_into().
     */
    static size_t estimate_output_size(const audio_spec& src_spec,
                                       size_t src_len,
                                       const audio_spec& dst_spec);
    
    /**
     * @brief Convert into provided buffer
     * @param src_spec Source specification
     * @param src_data Source audio data
     * @param src_len Source length in bytes
     * @param dst_spec Target specification
     * @param dst_buffer Output buffer (resized if needed)
     * @return Bytes written to dst_buffer
     * @throws audio_conversion_error on failure
     * 
     * Converts audio into existing buffer, resizing if necessary.
     * More efficient than convert() when reusing buffers.
     */
    static size_t convert_into(const audio_spec& src_spec,
                               const uint8_t* src_data,
                               size_t src_len,
                               const audio_spec& dst_spec,
                               buffer<uint8_t>& dst_buffer);
    
    /**
     * @class stream_converter
     * @brief Stateful converter for streaming audio
     * 
     * Processes audio in chunks without requiring the entire
     * file in memory. Maintains internal state for resampling.
     */
    class stream_converter {
    public:
        /**
         * @brief Construct streaming converter
         * @param from Source audio specification
         * @param to Target audio specification
         */
        stream_converter(const audio_spec& from, const audio_spec& to);
        
        /**
         * @brief Destructor
         */
        ~stream_converter();
        
        /**
         * @brief Process audio chunk
         * @param input Input audio data
         * @param input_len Input length in bytes
         * @param output Output buffer (resized as needed)
         * @return Bytes written to output
         * 
         * Converts a chunk of audio. May buffer samples internally
         * for resampling.
         */
        size_t process_chunk(const uint8_t* input, size_t input_len,
                           buffer<uint8_t>& output);
        
        /**
         * @brief Flush buffered samples
         * @param output Output buffer
         * @return Bytes written to output
         * 
         * Call after processing all chunks to retrieve any
         * remaining buffered samples.
         */
        size_t flush(buffer<uint8_t>& output);
        
        /**
         * @brief Reset converter state
         * 
         * Clears internal buffers and resampling state.
         */
        void reset();
        
    private:
        struct impl;
        std::unique_ptr<impl> m_pimpl;
    };
};

/**
 * @class audio_conversion_error
 * @brief Base exception for conversion errors
 * @ingroup sdk_conversion
 */
class audio_conversion_error : public std::runtime_error {
public:
    /**
     * @brief Construct with error message
     * @param msg Error description
     */
    explicit audio_conversion_error(const std::string& msg) 
        : std::runtime_error("Audio conversion error: " + msg) {}
};

/**
 * @class unsupported_format_error
 * @brief Exception for unsupported format conversions
 * @ingroup sdk_conversion
 * 
 * Thrown when attempting to convert to/from an unsupported
 * audio format.
 */
class unsupported_format_error : public audio_conversion_error {
public:
    /**
     * @brief Construct with format
     * @param fmt Unsupported audio format
     */
    explicit unsupported_format_error(audio_format fmt) 
        : audio_conversion_error("Unsupported audio format: " + std::to_string(static_cast<int>(fmt))) {}
};

} // namespace musac

#endif // MUSAC_SDK_AUDIO_CONVERTER_V2_HH