/**
 * @file decoder_8svx.hh
 * @brief IFF 8SVX (8-bit Sampled Voice) decoder for Amiga audio
 * @ingroup codecs
 */

#ifndef MUSAC_DECODER_8SVX_HH
#define MUSAC_DECODER_8SVX_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    
    /**
     * @class decoder_8svx
     * @brief Decoder for IFF 8SVX (Amiga 8-bit Sampled Voice) format
     * @ingroup codecs
     * 
     * The 8SVX format is an IFF-based format designed for the Amiga computer,
     * storing 8-bit audio samples with support for instruments and compression.
     * 
     * ## Format Features
     * 
     * ### Audio Structure
     * - **8-bit signed samples** stored in big-endian IFF chunks
     * - **One-shot samples**: Initial transient waveform
     * - **Repeat samples**: Looping waveform for sustained notes
     * - **Octave-based storage**: Multiple octaves with 2:1 size ratio
     * 
     * ### Compression
     * - **None**: Uncompressed 8-bit PCM
     * - **Fibonacci-delta**: Delta encoding with Fibonacci sequence
     * 
     * ## Chunk Support
     * 
     * ### Required Chunks
     * - **FORM**: Container with "8SVX" type
     * - **VHDR**: Voice header with playback parameters
     * - **BODY**: Sample data
     * 
     * ### Optional Chunks
     * - **NAME**: Sample name
     * - **AUTH**: Author information
     * - **(c)**: Copyright notice
     * - **ANNO**: Annotations
     * - **ATAK**: Attack envelope
     * - **RLSE**: Release envelope
     * 
     * ## Playback Parameters
     * 
     * The VHDR chunk contains:
     * - Number of one-shot samples
     * - Number of repeat samples
     * - Samples per cycle (for pitched playback)
     * - Playback sample rate
     * - Number of octaves
     * - Compression type
     * - Volume (0 to Unity)
     * 
     * @see decoder_aiff_v2 for standard AIFF support
     */
    class MUSAC_CODECS_EXPORT decoder_8svx : public decoder {
    public:
        decoder_8svx();
        ~decoder_8svx() override;
        
        /**
         * @brief Check if stream contains 8SVX data
         * @param rwops Input stream to check
         * @return true if stream contains 8SVX format
         * 
         * Checks for FORM chunk with "8SVX" type and valid VHDR.
         */
        [[nodiscard]] static bool accept(io_stream* rwops);
        
        /**
         * @brief Get decoder name
         * @return "8SVX" string
         */
        [[nodiscard]] const char* get_name() const override;
        
        /**
         * @brief Open and parse 8SVX file
         * @param rwops Input stream containing 8SVX data
         * @throws decoder_error if not a valid 8SVX file
         * @throws format_error if compression not supported
         * 
         * Parses VHDR and prepares sample data for playback.
         */
        void open(io_stream* rwops) override;
        
        /**
         * @brief Get number of audio channels
         * @return Always 1 (8SVX is mono-only)
         */
        [[nodiscard]] channels_t get_channels() const override;
        
        /**
         * @brief Get sample rate
         * @return Sample rate from VHDR
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
         * 
         * For one-shot samples, returns duration of one-shot portion.
         * For instruments, returns one-shot + one repeat cycle.
         */
        [[nodiscard]] std::chrono::microseconds duration() const override;
        
        /**
         * @brief Seek to specific time position
         * @param pos Target position in microseconds
         * @return true if seek successful
         */
        bool seek_to_time(std::chrono::microseconds pos) override;
        
        // 8SVX-specific API
        
        /**
         * @brief Check if sample has repeat portion
         * @return true if repeat samples > 0
         */
        [[nodiscard]] bool has_repeat() const;
        
        /**
         * @brief Get number of octaves
         * @return Octave count from VHDR
         */
        [[nodiscard]] uint8_t get_octave_count() const;
        
        /**
         * @brief Check if Fibonacci-delta compression is used
         * @return true if compressed
         */
        [[nodiscard]] bool is_compressed() const;
        
        /**
         * @brief Get playback volume
         * @return Volume from 0.0 to 1.0
         */
        [[nodiscard]] float get_volume() const;
        
        /**
         * @brief Check if envelope data is present
         * @return true if ATAK or RLSE chunks found
         */
        [[nodiscard]] bool has_envelope() const;

    protected:
        /**
         * @brief Decode audio samples
         * @param buf Output buffer for decoded samples
         * @param len Buffer size in samples
         * @param call_again Set to true if more data available
         * @return Number of samples decoded
         * 
         * Handles one-shot/repeat playback and decompression.
         * Converts 8-bit signed to float output.
         */
        size_t do_decode(float* buf, size_t len, bool& call_again) override;

    private:
        struct impl;
        std::unique_ptr<impl> m_pimpl;
    };
    
} // namespace musac

#endif // MUSAC_DECODER_8SVX_HH