/**
 * @file decoder_aiff.hh
 * @brief AIFF (Audio Interchange File Format) decoder
 * @ingroup codecs
 */

// Created by igor on 3/18/25.

#ifndef  DECODER_AIFF_HH
#define  DECODER_AIFF_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    
    /**
     * @class decoder_aiff
     * @brief Decoder for AIFF (Audio Interchange File Format) files
     * @ingroup codecs
     * 
     * The AIFF decoder provides support for reading Audio Interchange File Format
     * files, a standard audio format developed by Apple. AIFF is an uncompressed
     * format that stores audio data in big-endian byte order.
     * 
     * ## Format Support
     * 
     * - **Sample formats**: 8-bit, 16-bit, 24-bit, 32-bit PCM
     * - **Sample rates**: All standard rates (8kHz - 192kHz)
     * - **Channels**: Mono, stereo, and multi-channel
     * - **Compression**: Uncompressed PCM only
     * - **Variants**: AIFF and AIFF-C (AIFC)
     * 
     * ## AIFF Structure
     * 
     * AIFF files use the IFF (Interchange File Format) chunk structure:
     * ```
     * FORM chunk
     * ├── COMM chunk (Common - format info)
     * ├── SSND chunk (Sound data)
     * └── Optional chunks (NAME, AUTH, ANNO, etc.)
     * ```
     * 
     * ## Usage Example
     * 
     * @code
     * // Direct usage
     * auto stream = io_from_file("audio.aiff", "rb");
     * decoder_aiff decoder;
     * decoder.open(stream.get());
     * 
     * std::cout << "Channels: " << decoder.get_channels() << '\n';
     * std::cout << "Sample rate: " << decoder.get_rate() << " Hz\n";
     * std::cout << "Duration: " << decoder.duration().count() << " µs\n";
     * 
     * // Decode audio
     * std::vector<float> buffer(4096);
     * bool more = true;
     * while (more) {
     *     size_t decoded = decoder.decode(buffer.data(), buffer.size(), more);
     *     // Process decoded samples...
     * }
     * @endcode
     * 
     * ## Format Detection
     * 
     * The decoder identifies AIFF files by:
     * 1. "FORM" signature at offset 0
     * 2. "AIFF" or "AIFC" type at offset 8
     * 
     * @code
     * if (decoder_aiff::accept(stream.get())) {
     *     // This is an AIFF file
     *     decoder_aiff decoder;
     *     decoder.open(stream.get());
     * }
     * @endcode
     * 
     * ## Implementation Notes
     * 
     * - Uses big-endian byte order (Motorola format)
     * - Supports seeking to arbitrary positions
     * - Handles truncated files gracefully
     * - Skips unknown chunks
     * - Validates chunk sizes for safety
     * 
     * @see decoder_wav for similar WAV format support
     * @see https://en.wikipedia.org/wiki/Audio_Interchange_File_Format
     */
    class MUSAC_CODECS_EXPORT decoder_aiff : public decoder {
        public:
            decoder_aiff();
            ~decoder_aiff() override;
            
            /**
             * @brief Check if stream contains AIFF data
             * @param rwops Input stream to check
             * @return true if stream contains AIFF format
             * 
             * Checks for AIFF file signature without modifying stream position.
             * Looks for "FORM" header followed by "AIFF" or "AIFC" type.
             */
            [[nodiscard]] static bool accept(io_stream* rwops);
            
            /**
             * @brief Get decoder name
             * @return "AIFF" string
             */
            [[nodiscard]] const char* get_name() const override;
            
            /**
             * @brief Open and parse AIFF file
             * @param rwops Input stream containing AIFF data
             * @throws decoder_error if not a valid AIFF file
             * @throws format_error if unsupported AIFF variant
             * 
             * Parses AIFF header chunks and prepares for decoding.
             * Stream position is set to start of audio data.
             */
            void open(io_stream* rwops) override;
            
            /**
             * @brief Get number of audio channels
             * @return Channel count (1=mono, 2=stereo, etc.)
             */
            [[nodiscard]] channels_t get_channels() const override;
            
            /**
             * @brief Get sample rate
             * @return Sample rate in Hz
             */
            [[nodiscard]] sample_rate_t get_rate() const override;
            
            /**
             * @brief Reset to beginning of audio
             * @return true if successful
             */
            bool rewind() override;
            
            /**
             * @brief Get total duration
             * @return Duration in microseconds
             */
            [[nodiscard]] std::chrono::microseconds duration() const override;
            
            /**
             * @brief Seek to specific time position
             * @param pos Target position in microseconds
             * @return true if seek successful
             */
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
            /**
             * @brief Decode audio samples
             * @param buf Output buffer for decoded samples
             * @param len Buffer size in samples
             * @param call_again Set to true if more data available
             * @return Number of samples decoded
             * 
             * Decodes PCM samples from AIFF file and converts to float format.
             * Handles big-endian to native endian conversion automatically.
             */
            size_t do_decode(float* buf, size_t len, bool& call_again) override;

        private:
            struct impl; ///< Private implementation (PIMPL idiom)
            std::unique_ptr<impl> m_pimpl; ///< Pointer to implementation
    };
}

#endif
