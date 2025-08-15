/**
 * @file audio_source.hh
 * @brief Audio data source abstraction
 * @author Igor
 * @date 2025-03-23
 * @ingroup sources
 */

#ifndef  AUDIO_SOURCE_HH
#define  AUDIO_SOURCE_HH

#include <chrono>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/decoders_registry.hh>
#include <musac/sdk/resampler.hh>
#include <musac/sdk/types.hh>
#include <musac/export_musac.h>

namespace musac {
    /**
     * @class audio_source
     * @brief Provides audio data for streaming
     * @ingroup sources
     * 
     * The audio_source class represents a source of audio data that can be
     * played by an audio_stream. It combines a decoder (for reading audio
     * formats), an optional resampler (for sample rate conversion), and an
     * I/O stream (for data access).
     * 
     * ## Creating Sources
     * 
     * Sources are typically created using helper functions:
     * 
     * @code
     * // Load from file
     * auto source = audio_loader::load("music.mp3");
     * 
     * // Create from memory
     * std::vector<uint8_t> data = load_file_data();
     * auto source = audio_loader::load_from_memory(data);
     * 
     * // With automatic format detection
     * auto stream = io_from_file("audio.ogg");
     * audio_source source(std::move(stream));
     * @endcode
     * 
     * ## Manual Construction
     * 
     * For advanced use, sources can be constructed manually:
     * 
     * @code
     * auto stream = io_from_file("music.flac");
     * auto decoder = std::make_unique<decoder_drflac>();
     * auto resampler = std::make_unique<linear_resampler>();
     * 
     * audio_source source(
     *     std::move(decoder),
     *     std::move(resampler),
     *     std::move(stream)
     * );
     * @endcode
     * 
     * ## Ownership
     * 
     * audio_source takes ownership of the decoder, resampler, and I/O stream.
     * Sources are move-only and cannot be copied. When creating a stream from
     * a source, the source is moved into the stream:
     * 
     * @code
     * auto source = audio_loader::load("music.mp3");
     * auto stream = device.create_stream(std::move(source));
     * // source is now invalid and cannot be used
     * @endcode
     * 
     * @see decoder, resampler, io_stream, audio_stream
     */
    class MUSAC_EXPORT audio_source {
        public:
            /**
             * @brief Construct source with decoder and resampler
             * 
             * @param decoder_obj Decoder for reading audio format
             * @param resampler_obj Resampler for sample rate conversion
             * @param rwops I/O stream containing audio data
             */
            audio_source(std::unique_ptr <decoder> decoder_obj,
                         std::unique_ptr <resampler> resampler_obj,
                         std::unique_ptr<musac::io_stream> rwops);

            /**
             * @brief Construct source with decoder only (no resampling)
             * 
             * @param decoder_obj Decoder for reading audio format
             * @param rwops I/O stream containing audio data
             */
            audio_source(std::unique_ptr <decoder> decoder_obj,
                         std::unique_ptr<musac::io_stream> rwops);

            /**
             * @brief Construct with automatic format detection
             * 
             * Automatically detects the audio format and selects an appropriate
             * decoder from the registry.
             * 
             * @param rwops I/O stream containing audio data
             * @param registry Optional decoder registry (uses global if null)
             * 
             * @throws decoder_error if format cannot be detected
             * 
             * @code
             * auto stream = io_from_file("music.mp3");
             * audio_source source(std::move(stream));  // Auto-detects MP3
             * @endcode
             */
            audio_source(std::unique_ptr<musac::io_stream> rwops, 
                        const decoders_registry* registry = nullptr);

            /**
             * @brief Construct with automatic format detection and resampler
             * 
             * @param rwops I/O stream containing audio data
             * @param resampler_obj Resampler for sample rate conversion
             * @param registry Optional decoder registry (uses global if null)
             * 
             * @throws decoder_error if format cannot be detected
             */
            audio_source(std::unique_ptr<musac::io_stream> rwops,
                        std::unique_ptr<resampler> resampler_obj,
                        const decoders_registry* registry = nullptr);

            /**
             * @brief Move constructor
             */
            audio_source(audio_source&&) noexcept;
            
            /**
             * @brief Move assignment is deleted
             */
            audio_source& operator=(audio_source&&) noexcept = delete;

            /**
             * @brief Copy constructor is deleted (sources are move-only)
             */
            audio_source(const audio_source&) = delete;
            
            /**
             * @brief Copy assignment is deleted (sources are move-only)
             */
            audio_source& operator = (const audio_source&) = delete;

            /**
             * @brief Destructor
             */
            virtual ~audio_source();

            /**
             * @brief Rewind to the beginning
             * 
             * @return true if successful, false if not seekable
             * 
             * @note Not all sources support rewinding (e.g., network streams)
             */
            virtual bool rewind();

            /**
             * @brief Open and configure the source for playback
             * 
             * Called automatically by audio_stream when playback starts.
             * 
             * @param rate Target sample rate
             * @param channels Target channel count
             * @param frame_size Buffer frame size
             * 
             * @throws format_error if source cannot be configured
             */
            virtual void open(sample_rate_t rate, channels_t channels, size_t frame_size);

            /**
             * @brief Read audio samples
             * 
             * Reads and converts audio samples to float format. Called by
             * the audio callback to fill buffers.
             * 
             * @param[out] buf Output buffer for samples
             * @param[in,out] cur_pos Current position in buffer, updated after read
             * @param len Buffer length in samples
             * @param device_channels Number of device channels
             * 
             * @warning Called from audio thread - must be real-time safe
             */
            virtual void read_samples(float* buf, size_t& cur_pos, size_t len, channels_t device_channels);

            /**
             * @brief Get source duration
             * @return Duration, or zero if unknown
             * @see audio_stream::duration()
             */
            virtual std::chrono::microseconds duration() const;
            
            /**
             * @brief Seek to time position
             * @param pos Target position
             * @return true if successful, false if not seekable
             * @see audio_stream::seek_to_time()
             */
            virtual bool seek_to_time(std::chrono::microseconds pos) const;
        private:
            std::unique_ptr<musac::io_stream> m_rwops;
            // Resamplers hold a reference to decoders, so we store it as a shared_ptr.
            std::shared_ptr <decoder> m_decoder;
            std::unique_ptr <resampler> m_resampler;
    };
}

#endif
