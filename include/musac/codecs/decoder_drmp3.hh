/**
 * @file decoder_drmp3.hh
 * @brief MP3 (MPEG-1/2 Audio Layer III) decoder using dr_mp3
 * @ingroup codecs
 */

// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>
namespace musac {
    
    /**
     * @class decoder_drmp3
     * @brief MP3 decoder implementation using dr_mp3 library
     * @ingroup codecs
     * 
     * This decoder provides support for MP3 (MPEG-1/2 Audio Layer III) files
     * using the dr_mp3 single-header library. MP3 is a lossy audio compression
     * format that achieves high compression ratios while maintaining good quality.
     * 
     * ## Format Support
     * 
     * - **MPEG versions**: MPEG-1, MPEG-2, MPEG-2.5
     * - **Layers**: Layer III (MP3)
     * - **Bitrates**: 8-320 kbps (CBR), Variable (VBR)
     * - **Sample rates**: 8, 11.025, 12, 16, 22.05, 24, 32, 44.1, 48 kHz
     * - **Channels**: Mono, stereo, joint stereo, dual channel
     * - **Extensions**: ID3v1, ID3v2 metadata tags
     * 
     * ## MP3 Features
     * 
     * - **Lossy compression**: 10:1 typical compression ratio
     * - **Psychoacoustic model**: Removes inaudible frequencies
     * - **Variable bitrate**: Optimizes quality vs file size
     * - **Gapless playback**: Supports LAME gapless info
     * - **Wide compatibility**: Most supported audio format
     * 
     * ## Usage Example
     * 
     * @code
     * // Decode MP3 file
     * auto stream = io_from_file("song.mp3", "rb");
     * 
     * if (decoder_drmp3::accept(stream.get())) {
     *     decoder_drmp3 decoder;
     *     decoder.open(stream.get());
     *     
     *     // Get audio properties
     *     auto channels = decoder.get_channels();
     *     auto rate = decoder.get_rate();
     *     auto duration = decoder.duration();
     *     
     *     std::cout << "MP3: " << channels << " channels @ "
     *               << rate << " Hz, duration: "
     *               << std::chrono::duration_cast<std::chrono::seconds>(
     *                      duration).count() << " seconds\n";
     *     
     *     // Decode audio
     *     std::vector<float> buffer(4096);
     *     bool more = true;
     *     while (more) {
     *         size_t decoded = decoder.decode(buffer.data(), buffer.size(), more);
     *         // Process PCM samples...
     *     }
     * }
     * @endcode
     * 
     * ## VBR (Variable Bitrate) Support
     * 
     * The decoder handles VBR MP3 files transparently:
     * - Accurate duration calculation from VBR headers
     * - Proper seeking in VBR files
     * - Support for Xing/Info and VBRI headers
     * 
     * ## Performance Characteristics
     * 
     * - **Memory usage**: ~50KB decoder state
     * - **CPU usage**: 2-5% for typical stereo playback
     * - **Seek accuracy**: Frame-accurate for CBR, good for VBR
     * - **Startup time**: < 5ms for most files
     * 
     * ## Known Limitations
     * 
     * - No support for MP3 Pro or other extensions
     * - Seeking in VBR without headers may be slow
     * - Some very old MP3 encoders may not be supported
     * 
     * @note This decoder uses the dr_mp3 library by David Reid
     * @see https://github.com/mackron/dr_libs
     * @see decoder_vorbis for higher quality lossy compression
     */
    class MUSAC_CODECS_EXPORT decoder_drmp3 : public decoder {
        public:
            decoder_drmp3();
            ~decoder_drmp3() override;

            /**
             * @brief Check if stream contains MP3 data
             * @param rwops Input stream to check
             * @return true if stream contains MP3 format
             * 
             * Detects MP3 files by looking for:
             * - MPEG frame sync bits (0xFFE or 0xFFF)
             * - ID3v2 header at start
             * - Valid MPEG frame header
             */
            [[nodiscard]] static bool accept(io_stream* rwops);
            
            /**
             * @brief Get decoder name
             * @return "MP3 (dr_mp3)" string
             */
            [[nodiscard]] const char* get_name() const override;
            
            /**
             * @brief Open and initialize MP3 decoder
             * @param rwops Input stream containing MP3 data
             * @throws decoder_error if not a valid MP3 stream
             * @throws format_error if MP3 format is unsupported
             * 
             * Parses MP3 headers, skips ID3 tags, and prepares for decoding.
             * Detects CBR/VBR and calculates accurate duration if possible.
             */
            void open(io_stream* rwops) override;
            
            /**
             * @brief Get number of audio channels
             * @return 1 for mono, 2 for stereo
             */
            [[nodiscard]] channels_t get_channels() const override;
            
            /**
             * @brief Get sample rate
             * @return Sample rate in Hz (8000-48000)
             */
            [[nodiscard]] sample_rate_t get_rate() const override;
            
            /**
             * @brief Reset to beginning of audio
             * @return true if successful
             * 
             * Seeks back to first MP3 frame, skipping any ID3 headers.
             */
            bool rewind() override;
            
            /**
             * @brief Get total duration
             * @return Duration in microseconds
             * 
             * For CBR files, calculates from file size and bitrate.
             * For VBR files, uses Xing/VBRI headers if available.
             * May be approximate for VBR files without headers.
             */
            [[nodiscard]] std::chrono::microseconds duration() const override;
            
            /**
             * @brief Seek to specific time position  
             * @param pos Target position in microseconds
             * @return true if seek successful
             * 
             * Seeks to nearest MP3 frame. For VBR files, uses
             * seek table from Xing header if available.
             */
            auto seek_to_time(std::chrono::microseconds pos) -> bool override;

        protected:
            /**
             * @brief Decode MP3 audio samples
             * @param buf Output buffer for decoded samples
             * @param len Buffer size in samples
             * @param callAgain Set to true if more data available
             * @return Number of samples decoded
             * 
             * Decodes MP3 frames to 32-bit float PCM samples.
             * Handles frame boundaries and bit reservoir automatically.
             * Applies synthesis filterbank and inverse MDCT.
             */
            size_t do_decode(float buf[], size_t len, bool& callAgain) override;

        private:
            struct impl; ///< Private implementation details
            std::unique_ptr <impl> m_pimpl; ///< PIMPL for dr_mp3 state
    };
}
