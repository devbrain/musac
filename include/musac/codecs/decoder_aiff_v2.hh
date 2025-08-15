/**
 * @file decoder_aiff_v2.hh
 * @brief Modern AIFF/AIFF-C decoder using libiff
 * @ingroup codecs
 */

#ifndef MUSAC_DECODER_AIFF_V2_HH
#define MUSAC_DECODER_AIFF_V2_HH

#include <memory>
#include <vector>
#include <string>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    
    /**
     * @class decoder_aiff_v2
     * @brief Modern AIFF/AIFF-C decoder implementation using libiff
     * @ingroup codecs
     * 
     * This decoder provides comprehensive support for AIFF and AIFF-C formats
     * using the libiff library for proper IFF chunk parsing. It replaces the
     * legacy decoder_aiff with improved format support and validation.
     * 
     * ## Format Support
     * 
     * ### AIFF (Audio Interchange File Format)
     * - **Sample formats**: 8-bit, 16-bit, 24-bit, 32-bit PCM
     * - **Sample rates**: All rates via 80-bit IEEE extended precision
     * - **Channels**: Mono, stereo, quad, 5.1, and arbitrary channel counts
     * 
     * ### AIFF-C (Compressed Audio)
     * - **NONE**: Uncompressed PCM
     * - **ulaw/alaw**: G.711 compression
     * - **fl32/fl64**: IEEE floating point
     * - **ima4**: IMA ADPCM compression
     * 
     * ## Chunk Support
     * 
     * ### Required Chunks
     * - **FORM**: Container chunk with AIFF/AIFC type
     * - **COMM**: Common chunk with format parameters
     * - **SSND**: Sound data chunk
     * 
     * ### Optional Chunks
     * - **FVER**: Format version (AIFF-C)
     * - **MARK**: Markers for cue points
     * - **INST**: Instrument parameters
     * - **COMT**: Comments with timestamps
     * - **APPL**: Application-specific data
     * 
     * ## Key Features
     * 
     * - Proper IFF structure parsing via libiff
     * - Accurate 80-bit float sample rate conversion
     * - Streaming support for large files
     * - Robust error handling for malformed files
     * - FFmpeg-validated output accuracy
     * 
     * @see decoder_8svx for Amiga 8SVX format support
     */
    class MUSAC_CODECS_EXPORT decoder_aiff_v2 : public decoder {
    public:
        decoder_aiff_v2();
        ~decoder_aiff_v2() override;
        
        /**
         * @brief Check if stream contains AIFF/AIFF-C data
         * @param rwops Input stream to check
         * @return true if stream contains AIFF or AIFF-C format
         * 
         * Detects both AIFF and AIFF-C formats by checking for:
         * - FORM chunk with AIFF or AIFC type
         * - Valid COMM chunk presence
         */
        [[nodiscard]] static bool accept(io_stream* rwops);
        
        /**
         * @brief Get decoder name
         * @return "AIFF" for AIFF files, "AIFF-C" for compressed
         */
        [[nodiscard]] const char* get_name() const override;
        
        /**
         * @brief Open and parse AIFF/AIFF-C file
         * @param rwops Input stream containing AIFF data
         * @throws decoder_error if not a valid AIFF file
         * @throws format_error if unsupported format or compression
         * 
         * Parses all chunks and prepares for decoding.
         * Validates chunk structure and format parameters.
         */
        void open(io_stream* rwops) override;
        
        /**
         * @brief Get number of audio channels
         * @return Channel count from COMM chunk
         */
        [[nodiscard]] channels_t get_channels() const override;
        
        /**
         * @brief Get sample rate
         * @return Sample rate in Hz (from 80-bit float)
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
         * 
         * Uses MARK chunks for efficient seeking if available.
         * Falls back to sample-accurate calculation.
         */
        bool seek_to_time(std::chrono::microseconds pos) override;
        
        // Extended API for AIFF-specific features
        
        /**
         * @brief Check if file is AIFF-C (compressed)
         * @return true for AIFF-C, false for standard AIFF
         */
        [[nodiscard]] bool is_compressed() const;
        
        /**
         * @brief Get compression type for AIFF-C files
         * @return FourCC compression type or "NONE"
         */
        [[nodiscard]] uint32_t get_compression_type() const;
        
        /**
         * @brief Get marker information if available
         * @param marker_id Marker ID to query
         * @return Sample position of marker, or -1 if not found
         */
        [[nodiscard]] int64_t get_marker_position(uint16_t marker_id) const;
        
        /**
         * @brief Check if file contains instrument parameters
         * @return true if INST chunk is present
         */
        [[nodiscard]] bool has_instrument_data() const;
        
        /**
         * @brief Get MIDI base note for the sample
         * @return MIDI note number (0-127, middle C = 60), or -1 if no instrument data
         */
        [[nodiscard]] int get_base_note() const;
        
        /**
         * @brief Get detune value in cents
         * @return Detune in cents (-50 to +50), or 0 if no instrument data
         */
        [[nodiscard]] int get_detune() const;
        
        /**
         * @brief Get playback gain in decibels
         * @return Gain in dB, or 0 if no instrument data
         */
        [[nodiscard]] int get_gain_db() const;
        
        /**
         * @brief Get sustain loop information
         * @param[out] mode Loop mode (0=off, 1=forward, 2=pingpong)
         * @param[out] start_marker_id Marker ID for loop start
         * @param[out] end_marker_id Marker ID for loop end
         * @return true if sustain loop is defined
         */
        [[nodiscard]] bool get_sustain_loop(int& mode, uint16_t& start_marker_id, uint16_t& end_marker_id) const;
        
        /**
         * @brief Get release loop information
         * @param[out] mode Loop mode (0=off, 1=forward, 2=pingpong)
         * @param[out] start_marker_id Marker ID for loop start
         * @param[out] end_marker_id Marker ID for loop end
         * @return true if release loop is defined
         */
        [[nodiscard]] bool get_release_loop(int& mode, uint16_t& start_marker_id, uint16_t& end_marker_id) const;
        
        /**
         * @brief Get all marker IDs in the file
         * @return Vector of marker IDs
         */
        [[nodiscard]] std::vector<uint16_t> get_marker_ids() const;
        
        /**
         * @brief Get marker name
         * @param marker_id Marker ID to query
         * @return Marker name, or empty string if not found
         */
        [[nodiscard]] std::string get_marker_name(uint16_t marker_id) const;

    protected:
        /**
         * @brief Decode audio samples
         * @param buf Output buffer for decoded samples
         * @param len Buffer size in samples
         * @param call_again Set to true if more data available
         * @return Number of samples decoded
         * 
         * Handles decompression for AIFF-C formats.
         * Converts from source format to float output.
         */
        size_t do_decode(float* buf, size_t len, bool& call_again) override;

    private:
        struct impl;
        std::unique_ptr<impl> m_pimpl;
    };
    
} // namespace musac

#endif // MUSAC_DECODER_AIFF_V2_HH