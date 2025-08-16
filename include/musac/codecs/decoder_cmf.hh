/**
 * @file decoder_cmf.hh
 * @brief Creative Music File (CMF) format decoder
 * @ingroup codecs
 */

#ifndef  DECODER_CMF_HH
#define  DECODER_CMF_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    /*!
     * \brief Creative Music File (CMF) format decoder
     * \ingroup codecs
     * 
     * CMF (Creative Music File) is a MIDI-like music format created by Creative Labs
     * for use with their Sound Blaster cards. It was designed specifically for FM
     * synthesis using OPL2/OPL3 chips (Yamaha YM3812/YMF262) and was commonly used
     * in DOS games from the late 1980s to mid-1990s.
     * 
     * ## Format Features
     * - Based on Standard MIDI File (SMF) format
     * - Contains instrument definitions for FM synthesis
     * - 16 melodic channels + 5 percussion channels (OPL2 mode)
     * - 18 melodic channels + 5 percussion channels (OPL3 mode)
     * - Tempo and timing compatible with MIDI
     * - Smaller file sizes than General MIDI
     * 
     * ## File Structure
     * CMF files consist of:
     * - **Header**: Version, instrument offsets, tempo settings
     * - **Instrument Block**: FM synthesis parameters for each instrument
     * - **Music Data**: MIDI-like events with timing information
     * 
     * ## FM Synthesis Parameters
     * Each instrument definition includes:
     * - Modulator/Carrier settings
     * - Attack/Decay/Sustain/Release envelopes
     * - Waveform selection
     * - Frequency multiplication
     * - Feedback and connection algorithm
     * - Key scaling and level scaling
     * 
     * ## Supported Features
     * - All CMF versions (1.0, 1.1)
     * - OPL2 (mono) and OPL3 (stereo) emulation
     * - Full ADSR envelope support
     * - All waveform types
     * - Percussion mode
     * - Pitch bend and modulation
     * - Volume and expression control
     * 
     * ## Historical Context
     * CMF was used extensively in:
     * - DOS games (Duke Nukem, Commander Keen, Wolf3D)
     * - Creative Labs demos and utilities
     * - Early multimedia applications
     * - Educational software
     * 
     * The format competed with:
     * - ROL files (AdLib)
     * - IMF files (id Software)
     * - MUS files (various games)
     * 
     * ## Playback Characteristics
     * - Output: 44100 Hz stereo
     * - High-quality OPL3 emulation
     * - Authentic FM synthesis sound
     * - Low CPU usage
     * 
     * ## Example Usage
     * \code{.cpp}
     * auto io = musac::io_from_file("music.cmf");
     * musac::decoder_cmf decoder;
     * decoder.open(io.get());
     * 
     * std::cout << "Channels: " << decoder.get_channels() << "\n";
     * std::cout << "Sample rate: " << decoder.get_rate() << " Hz\n";
     * 
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
     * \note CMF provides authentic AdLib/Sound Blaster FM synthesis sound,
     * characteristic of DOS-era gaming.
     * 
     * \see decoder Base class for all decoders
     * \see decoder_opb For OPL instrument banks
     * \see decoders_registry For automatic format detection
     */
    class MUSAC_CODECS_EXPORT decoder_cmf : public decoder {
        public:
            decoder_cmf();
            ~decoder_cmf() override;
            
            /*!
             * \brief Check if a stream contains CMF data
             * \param rwops The stream to check
             * \return true if the stream contains CMF data
             * 
             * Checks for the CMF file signature "CTMF" at the beginning
             * of the stream. The stream position is restored after checking.
             */
            [[nodiscard]] static bool accept(io_stream* rwops);
            
            /*!
             * \brief Get the decoder name
             * \return "CMF" string
             */
            [[nodiscard]] const char* get_name() const override;
            
            /*!
             * \brief Open a CMF file for decoding
             * \param rwops The stream containing CMF data
             * \throws musac::decoder_error if the file is not a valid CMF file
             * 
             * Reads the CMF header, loads instrument definitions, parses
             * the music data, and initializes the OPL emulator. Determines
             * whether to use OPL2 or OPL3 emulation based on file version.
             */
            void open(io_stream* rwops) override;
            
            /*!
             * \brief Get the number of audio channels
             * \return 2 for stereo (OPL3) or converted to stereo from mono (OPL2)
             * 
             * Always returns stereo output for consistency, even when
             * emulating OPL2 which is natively mono.
             */
            [[nodiscard]] channels_t get_channels() const override;
            
            /*!
             * \brief Get the sample rate
             * \return Sample rate in Hz (typically 44100)
             */
            [[nodiscard]] sample_rate_t get_rate() const override;
            
            /*!
             * \brief Reset playback to the beginning
             * \return true on success
             * 
             * Resets the OPL emulator state and returns to the start
             * of the music data.
             */
            bool rewind() override;
            
            /*!
             * \brief Get the total duration of the CMF file
             * \return Duration in microseconds
             * 
             * Calculates duration based on MIDI timing information
             * and tempo changes throughout the file.
             */
            [[nodiscard]] std::chrono::microseconds duration() const override;
            
            /*!
             * \brief Seek to a specific time position
             * \param pos Target position in microseconds
             * \return true on success, false if seeking is not supported
             * 
             * \note CMF seeking requires processing all events up to the
             * target position to maintain correct instrument state.
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
             * Processes MIDI events, updates OPL registers, and generates
             * FM synthesized audio. Handles tempo changes, note events,
             * and controller changes.
             */
            size_t do_decode(float buf[], size_t len, bool& call_again) override;
        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}


#endif
