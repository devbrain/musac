/**
 * @file decoder.hh
 * @brief Base class for audio format decoders
 * @ingroup decoder_interface
 */

// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <musac/sdk/io_stream.hh>
#include <musac/sdk/export_musac_sdk.h>
#include <musac/sdk/types.hh>
#include <chrono>
#include <memory>

namespace musac {
    /**
     * @class decoder
     * @brief Abstract base class for all audio format decoders
     * @ingroup decoder_interface
     * 
     * The decoder class defines the interface that all audio format decoders
     * must implement. Decoders read compressed or encoded audio data and
     * convert it to floating-point PCM samples.
     * 
     * ## Implementing a Decoder
     * 
     * To create a new decoder:
     * 
     * @code
     * class my_decoder : public decoder {
     * public:
     *     const char* get_name() const override {
     *         return "My Format Decoder";
     *     }
     *     
     *     void open(io_stream* stream) override {
     *         // Parse header, initialize decoder
     *         m_stream = stream;
     *         parse_header();
     *         set_is_open(true);
     *     }
     *     
     *     channels_t get_channels() const override {
     *         return m_channels;
     *     }
     *     
     *     sample_rate_t get_rate() const override {
     *         return m_sample_rate;
     *     }
     *     
     * protected:
     *     size_t do_decode(float* buf, size_t len, bool& call_again) override {
     *         // Decode audio data to float samples
     *         size_t decoded = 0;
     *         // ... decoding logic ...
     *         call_again = !reached_end;
     *         return decoded;
     *     }
     * };
     * @endcode
     * 
     * ## Decoder Lifecycle
     * 
     * 1. Construction - Decoder is created
     * 2. open() - Initialize with I/O stream
     * 3. get_channels()/get_rate() - Query format
     * 4. decode() - Repeatedly called to get audio
     * 5. seek_to_time()/rewind() - Optional seeking
     * 6. Destruction - Cleanup
     * 
     * ## Format Detection
     * 
     * Decoders can optionally implement format detection via the
     * accept() method (see derived classes).
     * 
     * @see io_stream, audio_source, decoders_registry
     */
    class MUSAC_SDK_EXPORT decoder {
        public:
            /**
             * @brief Constructor
             */
            decoder();
            
            /**
             * @brief Virtual destructor
             */
            virtual ~decoder();

            /**
             * @brief Check if decoder is open and ready
             * @return true if decoder has been successfully opened
             */
            [[nodiscard]] bool is_open() const;
            
            /**
             * @brief Decode audio data to float samples
             * 
             * This is the main decoding function called by the audio system.
             * It handles channel conversion if needed.
             * 
             * @param[out] buf Buffer to fill with decoded samples
             * @param len Buffer size in samples
             * @param[out] call_again Set to true if more data is available
             * @param device_channels Number of channels in output device
             * @return Number of samples actually decoded
             * 
             * @note This calls the protected do_decode() internally
             */
            [[nodiscard]] size_t decode(float buf[], size_t len, bool& call_again, channels_t device_channels);
            
            /**
             * @brief Get human-readable name of this decoder
             * @return Decoder name (e.g., "FLAC Decoder", "MP3 Decoder")
             */
            [[nodiscard]] virtual const char* get_name() const = 0;

            /**
             * @brief Open and initialize the decoder
             * 
             * Parses the file header and prepares for decoding.
             * 
             * @param rwops I/O stream containing encoded audio data
             * @throws decoder_error if format is invalid or unsupported
             * 
             * @note The decoder does not take ownership of the stream
             */
            virtual void open(io_stream* rwops) = 0;
            
            /**
             * @brief Get number of audio channels
             * @return Channel count (1=mono, 2=stereo, etc.)
             * @pre Decoder must be open
             */
            [[nodiscard]] virtual channels_t get_channels() const = 0;
            
            /**
             * @brief Get sample rate
             * @return Sample rate in Hz (e.g., 44100, 48000)
             * @pre Decoder must be open
             */
            [[nodiscard]] virtual sample_rate_t get_rate() const = 0;
            
            /**
             * @brief Rewind to beginning of audio
             * @return true if successful, false if not seekable
             */
            virtual bool rewind() = 0;
            
            /**
             * @brief Get total duration of audio
             * @return Duration, or zero if unknown/unlimited
             * @note May be inaccurate for some formats (VBR MP3, MOD)
             */
            [[nodiscard]] virtual std::chrono::microseconds duration() const = 0;
            
            /**
             * @brief Seek to specific time position
             * @param pos Target position from start
             * @return true if successful, false if not seekable
             * @note Accuracy depends on format (sample-accurate for PCM, frame-aligned for MP3)
             */
            virtual bool seek_to_time(std::chrono::microseconds pos) = 0;

        protected:
            /**
             * @brief Set the open state
             * @param f true if decoder is successfully opened
             * 
             * Call this from your open() implementation after successful initialization.
             */
            void set_is_open(bool f);
            
            /**
             * @brief Implementation-specific decode function
             * 
             * Override this to implement actual decoding logic.
             * 
             * @param[out] buf Buffer to fill with float samples
             * @param len Buffer size in samples
             * @param[out] call_again Set to true if more data available
             * @return Number of samples decoded
             * 
             * @note Samples should be in range [-1.0, 1.0]
             * @note This is called by the public decode() method
             */
            virtual size_t do_decode(float* buf, size_t len, bool& call_again) = 0;

        private:
            struct impl;
            const std::unique_ptr <impl> m_pimpl;
    };


} // namespace Aulib

