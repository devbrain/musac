/**
 * @file decoder_opb.hh
 * @brief OPL Bank (OPB) format decoder
 * @ingroup codecs
 */

#ifndef  DECODER_OPB_HH
#define  DECODER_OPB_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    /*!
     * \brief OPL Bank (OPB) format decoder
     * \ingroup codecs
     * 
     * OPB (OPL Bank) files contain instrument definitions for OPL2/OPL3 FM synthesis
     * chips. These files store FM synthesis parameters that define how instruments
     * sound when played through Yamaha OPL-series sound chips (YM3812/YMF262).
     * 
     * ## Format Purpose
     * OPB files serve as instrument libraries for:
     * - FM synthesis music playback
     * - MIDI-to-OPL conversion
     * - Game music systems
     * - Sound effect generation
     * 
     * ## Instrument Parameters
     * Each instrument in an OPB bank contains:
     * 
     * ### Operator Settings (Modulator & Carrier)
     * - **ADSR Envelope**: Attack, Decay, Sustain, Release rates
     * - **Waveform**: Sine, half-sine, absolute-sine, pulse
     * - **Frequency Multiplier**: 0.5x to 15x
     * - **Key Scaling**: Rate and level scaling
     * - **Total Level**: Output amplitude
     * - **Tremolo/Vibrato**: Enable flags and depth
     * 
     * ### Algorithm Settings
     * - **Connection**: FM (serial) or AM (parallel)
     * - **Feedback**: Self-modulation amount (0-7)
     * - **Fine Tuning**: Pitch adjustment
     * 
     * ## Bank Organization
     * - **128 Melodic Instruments**: Mapped to General MIDI programs
     * - **128 Percussion Instruments**: Drum kit sounds
     * - **Custom Banks**: Game-specific instrument sets
     * 
     * ## Common OPB Files
     * - **GENMIDI.OP2**: Default DOOM/Duke3D instruments
     * - **STANDARD.OPB**: General MIDI mapping
     * - **MT32.OPB**: Roland MT-32 emulation
     * - **Custom Game Banks**: Specific to various DOS games
     * 
     * ## File Structure
     * ```
     * Header:
     *   - Magic number/signature
     *   - Version information
     *   - Number of instruments
     *   - Bank metadata
     * 
     * Instruments:
     *   - Modulator parameters (13 bytes)
     *   - Carrier parameters (13 bytes)
     *   - Connection/algorithm byte
     *   - Fine tune/transpose data
     *   - Instrument name (optional)
     * ```
     * 
     * ## Playback Modes
     * This decoder can operate in several modes:
     * - **Test Mode**: Play individual instruments with test notes
     * - **Demo Mode**: Cycle through all instruments
     * - **MIDI Mode**: Use with MIDI files for OPL playback
     * 
     * ## OPL Chip Emulation
     * - Accurate OPL2/OPL3 emulation
     * - 9/18 channel polyphony (OPL2/OPL3)
     * - 4-operator mode support (OPL3)
     * - Stereo panning (OPL3 only)
     * 
     * ## Example Usage
     * \code{.cpp}
     * auto io = musac::io_from_file("GENMIDI.OP2");
     * musac::decoder_opb decoder;
     * decoder.open(io.get());
     * 
     * // OPB decoder generates a demo sequence
     * std::cout << "Channels: " << decoder.get_channels() << "\n";
     * std::cout << "Sample rate: " << decoder.get_rate() << " Hz\n";
     * 
     * // Play instrument demo
     * float buffer[4096];
     * bool more_data = true;
     * while (more_data) {
     *     size_t decoded = decoder.decode(buffer, 4096, more_data);
     *     // Process decoded samples...
     * }
     * \endcode
     * 
     * \note OPB files are typically used in conjunction with MIDI files
     * or game music engines rather than standalone playback.
     * 
     * \see decoder_cmf For CMF files that use OPB instruments
     * \see decoder Base class for all decoders
     * \see decoders_registry For automatic format detection
     */
    class MUSAC_CODECS_EXPORT decoder_opb : public decoder {
        public:
            decoder_opb();
            ~decoder_opb() override;
            
            /*!
             * \brief Check if a stream contains OPB data
             * \param rwops The stream to check
             * \return true if the stream contains OPB/OP2 data
             * 
             * Checks for OPB file signatures including various OP2 format
             * variants. The stream position is restored after checking.
             */
            [[nodiscard]] static bool accept(io_stream* rwops);
            
            /*!
             * \brief Get the decoder name
             * \return "OPB" string
             */
            [[nodiscard]] const char* get_name() const override;
            
            /*!
             * \brief Open an OPB file for decoding
             * \param rwops The stream containing OPB data
             * \throws musac::decoder_error if the file is not a valid OPB file
             * 
             * Loads all instrument definitions from the bank and initializes
             * the OPL emulator. Sets up a demo sequence that cycles through
             * all instruments for audition purposes.
             */
            void open(io_stream* rwops) override;
            
            /*!
             * \brief Get the number of audio channels
             * \return 2 for stereo output
             * 
             * Always returns stereo, with OPL3 providing true stereo
             * and OPL2 being converted from mono.
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
             * Restarts the instrument demo sequence from the first instrument.
             */
            bool rewind() override;
            
            /*!
             * \brief Get the total duration of the demo sequence
             * \return Duration in microseconds
             * 
             * Returns the duration of one complete cycle through all
             * instruments in the bank.
             */
            [[nodiscard]] std::chrono::microseconds duration() const override;
            
            /*!
             * \brief Seek to a specific time position
             * \param pos Target position in microseconds
             * \return true on success, false if position is out of range
             * 
             * Seeks to the specified position in the instrument demo sequence.
             */
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
            /*!
             * \brief Decode audio samples
             * \param buf Output buffer for decoded samples
             * \param len Number of samples to decode
             * \param callAgain Set to true if more data is available
             * \return Number of samples actually decoded
             * 
             * Generates FM synthesized audio demonstrating each instrument
             * in the bank. Plays test notes with each instrument in sequence.
             */
            size_t do_decode(float* buf, size_t len, bool& callAgain) override;

        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}

#endif
