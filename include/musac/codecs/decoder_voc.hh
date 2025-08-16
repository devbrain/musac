/**
 * @file decoder_voc.hh
 * @brief Creative Voice File (VOC) format decoder
 * @ingroup codecs
 */

#ifndef  DECODER_VOC_HH
#define  DECODER_VOC_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    /*!
     * \brief Creative Voice File (VOC) format decoder
     * \ingroup codecs
     * 
     * The VOC format was created by Creative Labs for use with Sound Blaster
     * sound cards in the late 1980s and early 1990s. It was one of the first
     * widely-used digital audio formats on PC platforms.
     * 
     * ## Format Features
     * - Sample rates: 5000-44100 Hz (typically 11025 or 22050 Hz)
     * - Bit depths: 8-bit unsigned, 16-bit signed, 4-bit ADPCM
     * - Channels: Mono or stereo
     * - Block-based structure with multiple data types
     * - Built-in compression support
     * 
     * ## Supported Block Types
     * - **Type 0**: Terminator block
     * - **Type 1**: Sound data (8-bit)
     * - **Type 2**: Sound data continuation
     * - **Type 3**: Silence block
     * - **Type 4**: Marker block
     * - **Type 5**: Text/ASCII block
     * - **Type 6**: Repeat start
     * - **Type 7**: Repeat end
     * - **Type 8**: Extended block (stereo support)
     * - **Type 9**: Sound data (16-bit)
     * 
     * ## Compression Support
     * - **Type 0**: 8-bit unsigned PCM (uncompressed)
     * - **Type 1**: 4-bit ADPCM (Creative ADPCM)
     * - **Type 2**: 2.6-bit ADPCM
     * - **Type 3**: 2-bit ADPCM
     * - **Type 4**: 16-bit signed PCM
     * - **Type 6**: A-law
     * - **Type 7**: μ-law
     * 
     * ## Historical Context
     * VOC files were commonly used in:
     * - MS-DOS games (Duke Nukem, Wolfenstein 3D, Doom)
     * - Sound Blaster demo programs
     * - Early Windows multimedia applications
     * - Voice recording applications
     * 
     * ## Limitations
     * - Maximum file size: 16MB (original format)
     * - Limited to 44.1kHz sample rate
     * - Stereo support only in later versions
     * - No metadata beyond text blocks
     * 
     * ## Example Usage
     * \code{.cpp}
     * auto io = musac::io_from_file("sound.voc");
     * musac::decoder_voc decoder;
     * decoder.open(io.get());
     * 
     * std::cout << "Channels: " << decoder.get_channels() << "\n";
     * std::cout << "Sample rate: " << decoder.get_rate() << " Hz\n";
     * 
     * // VOC files have finite duration
     * auto duration = decoder.duration();
     * std::cout << "Duration: " << duration.count() / 1000000.0 << " seconds\n";
     * 
     * // Decode and play
     * float buffer[4096];
     * bool more_data = true;
     * while (more_data) {
     *     size_t decoded = decoder.decode(buffer, 4096, more_data);
     *     // Process decoded samples...
     * }
     * \endcode
     * 
     * \note VOC format is largely obsolete but still encountered in
     * retro gaming and audio preservation contexts.
     * 
     * \see decoder Base class for all decoders
     * \see decoders_registry For automatic format detection
     */
    class MUSAC_CODECS_EXPORT decoder_voc : public decoder {
        public:
            decoder_voc();
            ~decoder_voc() override;
            
            /*!
             * \brief Check if a stream contains VOC data
             * \param rwops The stream to check
             * \return true if the stream contains VOC data
             * 
             * Checks for the VOC file signature "Creative Voice File\x1A".
             * The stream position is restored after checking.
             */
            [[nodiscard]] static bool accept(io_stream* rwops);
            
            /*!
             * \brief Get the decoder name
             * \return "VOC" string
             */
            [[nodiscard]] const char* get_name() const override;
            
            /*!
             * \brief Open a VOC file for decoding
             * \param rwops The stream containing VOC data
             * \throws musac::decoder_error if the file is not a valid VOC file
             * 
             * Reads the VOC header, parses the block structure, and prepares
             * for playback. The entire file structure is analyzed to determine
             * format parameters and duration.
             */
            void open(io_stream* rwops) override;
            
            /*!
             * \brief Get the number of audio channels
             * \return 1 for mono, 2 for stereo
             * 
             * Early VOC files are always mono. Stereo support was added
             * in later versions using Type 8 extended blocks.
             */
            [[nodiscard]] channels_t get_channels() const override;
            
            /*!
             * \brief Get the sample rate
             * \return Sample rate in Hz
             * 
             * VOC files can have varying sample rates per block, but this
             * returns the rate of the first sound data block.
             */
            [[nodiscard]] sample_rate_t get_rate() const override;
            
            /*!
             * \brief Reset playback to the beginning
             * \return true on success
             * 
             * Resets to the first sound data block in the file.
             */
            bool rewind() override;
            
            /*!
             * \brief Get the total duration of the VOC file
             * \return Duration in microseconds
             * 
             * Calculates duration based on all sound and silence blocks,
             * taking into account repeat blocks if present.
             */
            [[nodiscard]] std::chrono::microseconds duration() const override;
            
            /*!
             * \brief Seek to a specific time position
             * \param pos Target position in microseconds
             * \return true on success, false if position is out of range
             * 
             * Seeks to the appropriate block and position within the VOC file.
             * Handles block boundaries and varying sample rates correctly.
             */
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
            /*!
             * \brief Decode audio samples
             * \param buf Output buffer for decoded samples
             * \param len Number of samples to decode
             * \param call_again Set to true if more data is available
             * \return Number of samples actually decoded
             * 
             * Processes VOC blocks sequentially, handling different block types,
             * compression formats, and sample rates. Automatically handles
             * silence blocks, repeat loops, and format changes.
             */
            size_t do_decode(float* buf, size_t len, bool& call_again) override;

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}

#endif
