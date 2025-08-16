// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    /*!
     * \brief OGG Vorbis audio decoder using stb_vorbis
     * \ingroup codecs
     * 
     * This decoder provides support for OGG Vorbis audio files using the
     * stb_vorbis single-header library. OGG Vorbis is a fully open, patent-free,
     * professional audio encoding and streaming technology.
     * 
     * ## Features
     * - Full Vorbis I specification support
     * - Accurate seeking support
     * - Low memory footprint
     * - No external dependencies (stb_vorbis is embedded)
     * 
     * ## Supported Formats
     * - OGG Vorbis files (.ogg, .oga)
     * - Sample rates: 8kHz to 192kHz
     * - Channels: Mono, stereo, and multi-channel (up to 255 channels)
     * - Variable bitrate (VBR) and constant bitrate (CBR)
     * 
     * ## Memory Usage
     * The decoder uses pushdata mode for streaming, which provides:
     * - Low memory footprint (typically ~150KB for stereo files)
     * - Efficient streaming from disk or memory
     * - No need to load entire file into memory
     * 
     * ## Limitations
     * - Uses stb_vorbis which may have slightly lower quality than libvorbis
     * - Seeking may be slower for very large files
     * - No support for chained OGG streams
     * 
     * ## Example Usage
     * \code{.cpp}
     * auto io = musac::io_from_file("music.ogg");
     * musac::decoder_vorbis decoder;
     * decoder.open(io.get());
     * 
     * // Check file properties
     * std::cout << "Channels: " << decoder.get_channels() << "\n";
     * std::cout << "Sample rate: " << decoder.get_rate() << " Hz\n";
     * std::cout << "Duration: " << decoder.duration().count() / 1000000.0 << " seconds\n";
     * 
     * // Decode audio
     * float buffer[4096];
     * bool more_data = true;
     * while (more_data) {
     *     size_t decoded = decoder.decode(buffer, 4096, more_data);
     *     // Process decoded samples...
     * }
     * \endcode
     * 
     * \see decoder Base class for all decoders
     * \see decoders_registry For automatic format detection
     */
    class MUSAC_CODECS_EXPORT decoder_vorbis : public decoder {
        public:
            decoder_vorbis();
            ~decoder_vorbis() override;

            /*!
             * \brief Check if a stream contains OGG Vorbis data
             * \param rwops The stream to check
             * \return true if the stream contains OGG Vorbis data, false otherwise
             * 
             * This method checks for the OGG Vorbis signature at the current stream position.
             * The stream position is restored after checking.
             */
            [[nodiscard]] static bool accept(io_stream* rwops);
            
            /*!
             * \brief Get the decoder name
             * \return "Vorbis (stb_vorbis)"
             */
            [[nodiscard]] const char* get_name() const override;
            
            /*!
             * \brief Open an OGG Vorbis stream for decoding
             * \param rwops The stream containing OGG Vorbis data
             * \throws musac::decoder_error if the stream is not valid OGG Vorbis
             * 
             * Opens and initializes the Vorbis decoder. The stream must remain valid
             * for the lifetime of the decoder.
             */
            void open(io_stream* rwops) override;
            
            /*!
             * \brief Get the number of audio channels
             * \return Number of channels (1 for mono, 2 for stereo, etc.)
             */
            [[nodiscard]] channels_t get_channels() const override;
            
            /*!
             * \brief Get the sample rate
             * \return Sample rate in Hz (e.g., 44100, 48000)
             */
            [[nodiscard]] sample_rate_t get_rate() const override;
            
            /*!
             * \brief Reset playback to the beginning
             * \return true on success, false on failure
             */
            bool rewind() override;
            
            /*!
             * \brief Get the total duration of the audio
             * \return Duration in microseconds, or 0 if unknown
             * 
             * \note Duration calculation may be approximate for VBR files
             */
            [[nodiscard]] std::chrono::microseconds duration() const override;
            
            /*!
             * \brief Seek to a specific time position
             * \param pos Target position in microseconds
             * \return true on success, false on failure
             * 
             * Seeks to the specified position in the audio stream. The actual position
             * after seeking may not be exact due to frame boundaries.
             */
            auto seek_to_time(std::chrono::microseconds pos) -> bool override;

        protected:
            size_t do_decode(float buf[], size_t len, bool& callAgain) override;

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}