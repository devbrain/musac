/**
 * @file decoder_seq.hh
 * @brief SEQ (Sequencer) format decoder
 * @ingroup codecs
 */

#ifndef  DECODER_SEQ_HH
#define  DECODER_SEQ_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    /*!
     * \brief MIDI-like sequence format decoder
     * \ingroup codecs
     * 
     * This decoder handles various MIDI-like sequence formats commonly used in
     * DOS games and early multimedia applications. It provides OPL-based FM
     * synthesis for authentic retro game music playback.
     * 
     * ## Supported Formats
     * 
     * ### MIDI (.mid)
     * - Standard MIDI File format (Type 0 and Type 1)
     * - Multiple tracks with 16 channels
     * - Full General MIDI event support
     * - Used universally for music sequencing
     * 
     * ### MUS (.mus)
     * - id Software's simplified MIDI format
     * - Used in DOOM, DOOM II, Heretic, Hexen
     * - Smaller than MIDI with limited features
     * - 16 channels, subset of MIDI events
     * 
     * ### XMI (.xmi)
     * - Extended MIDI format by Miles Design
     * - Used in Origin games (Ultima, Wing Commander)
     * - Supports multiple tracks and branching
     * - Enhanced timing and looping features
     * 
     * ### HMI (.hmi)
     * - Human Machine Interfaces MIDI format
     * - Used in many DOS games (Descent, Blood)
     * - Multiple track support
     * - Custom controller mappings
     * 
     * ### HMP (.hmp)
     * - Human Machine Interfaces simplified format
     * - Variant of HMI with reduced features
     * - Used in some 3D Realms games
     * 
     * ## OPL Synthesis
     * This decoder uses OPL3 emulation for playback:
     * - Authentic FM synthesis sound
     * - GENMIDI patches (DOOM/Duke3D instruments)
     * - 18 channels of polyphony
     * - Stereo output support
     * 
     * ## Playback Characteristics
     * - **Sample Rate**: 44100 Hz
     * - **Channels**: Stereo
     * - **Synthesis**: OPL3 FM emulation
     * - **Patches**: GENMIDI.OP2 instrument bank
     * - **Polyphony**: Up to 18 simultaneous notes
     * 
     * ## Common Uses
     * These formats were extensively used in:
     * - DOS games (DOOM, Duke Nukem 3D, Descent)
     * - Early Windows games
     * - MIDI sequencing software
     * - Game music composition tools
     * 
     * ## Example Usage
     * \code{.cpp}
     * // Load a DOOM MUS file
     * auto io = musac::io_from_file("e1m1.mus");
     * musac::decoder_seq decoder;
     * decoder.open(io.get());
     * 
     * std::cout << "Format: " << decoder.get_name() << "\n";
     * std::cout << "Channels: " << decoder.get_channels() << "\n";
     * std::cout << "Sample rate: " << decoder.get_rate() << " Hz\n";
     * 
     * auto duration = decoder.duration();
     * if (duration.count() > 0) {
     *     std::cout << "Duration: " << duration.count() / 1000000.0 << " seconds\n";
     * } else {
     *     std::cout << "File loops indefinitely\n";
     * }
     * 
     * // Decode and play with OPL synthesis
     * float buffer[4096];
     * bool more_data = true;
     * while (more_data) {
     *     size_t decoded = decoder.decode(buffer, 4096, more_data);
     *     // Process decoded FM synthesized samples...
     * }
     * \endcode
     * 
     * \note This decoder provides authentic OPL-based playback for
     * retro game music. For modern General MIDI playback, consider
     * using a SoundFont-based MIDI decoder instead.
     * 
     * \see decoder Base class for all decoders
     * \see decoder_cmf For Creative's CMF format with similar OPL synthesis
     * \see decoders_registry For automatic format detection
     */
    class MUSAC_CODECS_EXPORT decoder_seq : public decoder {
        public:
            decoder_seq();
            ~decoder_seq() override;
            
            /*!
             * \brief Check if a stream contains supported sequence data
             * \param rwops The stream to check
             * \return true if the stream contains MID, MUS, XMI, HMI, or HMP data
             * 
             * Checks for format signatures:
             * - MID: "MThd" header
             * - MUS: "MUS\x1a" header
             * - XMI: "FORM" followed by "XDIR" or "XMID"
             * - HMI: "HMI-MIDISONG" header
             * - HMP: "HMIMIDIP" header
             * The stream position is restored after checking.
             */
            [[nodiscard]] static bool accept(io_stream* rwops);
            
            /*!
             * \brief Get the decoder name
             * \return "MIDI Sequence (MID/MUS/XMI/HMI/HMP)" string
             */
            [[nodiscard]] const char* get_name() const override;
            
            /*!
             * \brief Open a sequence file for decoding
             * \param rwops The stream containing sequence data
             * \throws musac::decoder_error if the file is not a valid sequence file
             * 
             * Loads the sequence data, initializes the OPL MIDI synthesizer
             * with GENMIDI patches, and prepares for playback. The format
             * is automatically detected from the file header.
             */
            void open(io_stream* rwops) override;
            
            /*!
             * \brief Get the number of audio channels
             * \return 2 for stereo output
             * 
             * Always returns stereo output regardless of the synthesis method.
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
             * Resets all tracks to the beginning and reinitializes
             * the synthesis engine state.
             */
            bool rewind() override;
            
            /*!
             * \brief Get the total duration of the sequence
             * \return Duration in microseconds, or 0 if the file loops
             * 
             * Calculates duration based on all MIDI events and tempo changes.
             * Some game music files are designed to loop and may return 0.
             */
            [[nodiscard]] auto duration() const -> std::chrono::microseconds override;
            
            /*!
             * \brief Seek to a specific time position
             * \param pos Target position in microseconds
             * \return true on success, false if seeking is not supported
             * 
             * \note Seeking in MIDI-like formats requires processing all events
             * up to the target position to maintain correct instrument and
             * controller state. This implementation currently does not support
             * seeking and always returns false.
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
             * Processes MIDI events through the OPL synthesizer to generate
             * FM synthesized audio. Handles note events, program changes,
             * controller changes, and timing. Produces authentic DOS game
             * music sound using GENMIDI instrument patches.
             */
            size_t do_decode(float buf[], size_t len, bool& call_again) override;
        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}



#endif
