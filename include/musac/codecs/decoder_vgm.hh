/**
 * @file decoder_vgm.hh
 * @brief Video Game Music (VGM) format decoder
 * @ingroup codecs
 */

#ifndef  DECODER_VGM_HH
#define  DECODER_VGM_HH

#include <memory>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>

namespace musac {
    /*!
     * \brief Video Game Music (VGM) format decoder
     * \ingroup codecs
     * 
     * The VGM (Video Game Music) format is a sample-accurate sound logging format
     * for many machines and sound chips. It logs the commands sent to the sound
     * chips, allowing for perfect reproduction of the original audio.
     * 
     * ## Supported Chip Emulations
     * 
     * ### Yamaha Chips
     * - **YM2612** (OPN2) - Sega Genesis/Mega Drive
     * - **YM2151** (OPM) - Arcade systems
     * - **YM2203** (OPN) - PC-88/PC-98
     * - **YM2608** (OPNA) - PC-88/PC-98
     * - **YM2610** (OPNB) - Neo Geo
     * - **YM2413** (OPLL) - Sega Master System (Japan)
     * - **YM3812** (OPL2) - AdLib/Sound Blaster
     * - **YM3526** (OPL) - Early PC sound cards
     * - **YMF262** (OPL3) - Sound Blaster 16
     * - **YMF278B** (OPL4) - MSX-AUDIO
     * 
     * ### Other Sound Chips
     * - **SN76489** (PSG) - Sega Master System, Game Gear
     * - **AY-3-8910** - ZX Spectrum, Amstrad CPC
     * - **Konami SCC** - MSX games
     * - **Nintendo GameBoy** - DMG sound
     * - **NES APU** - Nintendo Entertainment System
     * - **HuC6280** - PC Engine/TurboGrafx-16
     * - **Pokey** - Atari 8-bit computers
     * 
     * ## Format Versions
     * - VGM 1.00 to 1.71 supported
     * - GD3 tag support for metadata
     * - VGZ (gzipped VGM) support
     * 
     * ## Features
     * - Sample-accurate playback
     * - Loop point support (with loop count control)
     * - Dual chip support for enhanced music
     * - GD3 metadata tags (title, artist, game, etc.)
     * - Accurate chip emulation via various emulation cores
     * 
     * ## Playback Characteristics
     * - Output: 44100 Hz stereo by default
     * - High-quality resampling from chip native rates
     * - Optional low-pass filtering for authentic sound
     * 
     * ## Loop Support
     * VGM files often contain loop points for background music:
     * - Intro section plays once
     * - Loop section repeats indefinitely or specified times
     * - Seamless looping without gaps
     * 
     * ## Example Usage
     * \code{.cpp}
     * auto io = musac::io_from_file("sonic.vgm");
     * musac::decoder_vgm decoder;
     * decoder.open(io.get());
     * 
     * std::cout << "Channels: " << decoder.get_channels() << "\n";
     * std::cout << "Sample rate: " << decoder.get_rate() << " Hz\n";
     * 
     * // VGM files often loop
     * auto duration = decoder.duration();
     * if (duration.count() > 0) {
     *     std::cout << "Duration: " << duration.count() / 1000000.0 << " seconds\n";
     * } else {
     *     std::cout << "File loops indefinitely\n";
     * }
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
     * \note VGM files are commonly used for video game music preservation,
     * especially for retro gaming systems with programmable sound generators.
     * 
     * \see decoder Base class for all decoders
     * \see decoders_registry For automatic format detection
     */
    class MUSAC_CODECS_EXPORT decoder_vgm : public decoder {
        public:
            decoder_vgm();
            ~decoder_vgm() override;
            
            /*!
             * \brief Check if a stream contains VGM data
             * \param rwops The stream to check
             * \return true if the stream contains VGM or VGZ data
             * 
             * Checks for VGM file signature ("Vgm ") or gzip header for VGZ files.
             * The stream position is restored after checking.
             */
            [[nodiscard]] static bool accept(io_stream* rwops);
            
            /*!
             * \brief Get the decoder name
             * \return "VGM" string
             */
            [[nodiscard]] const char* get_name() const override;
            
            /*!
             * \brief Open a VGM file for decoding
             * \param rwops The stream containing VGM data
             * \throws musac::decoder_error if the file is not a valid VGM file
             * 
             * Reads the VGM header, initializes chip emulators, and prepares
             * for playback. Supports both VGM and VGZ (gzipped) formats.
             */
            void open(io_stream* rwops) override;
            
            /*!
             * \brief Get the number of audio channels
             * \return Always returns 2 (stereo output)
             * 
             * VGM playback always produces stereo output regardless of the
             * original chip configuration.
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
             * Resets all chip emulators and returns to the start of the VGM data.
             * For looped tracks, this restarts from the beginning, not the loop point.
             */
            bool rewind() override;
            
            /*!
             * \brief Get the total duration of the VGM file
             * \return Duration in microseconds, or 0 if the file loops indefinitely
             * 
             * For files with loop points, returns the duration of intro + one loop.
             * For non-looping files, returns the total playback time.
             * 
             * \note Files that loop forever without an end command return 0.
             */
            [[nodiscard]] std::chrono::microseconds duration() const override;
            
            /*!
             * \brief Seek to a specific time position
             * \param pos Target position in microseconds
             * \return true on success, false if seeking is not supported
             * 
             * \note VGM seeking is implemented by fast-forwarding through commands,
             * which may be slow for large files. Seeking backwards requires rewinding
             * and fast-forwarding from the start.
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
             * Processes VGM commands and generates audio samples from the
             * emulated sound chips. Handles timing, loops, and end-of-stream.
             */
            size_t do_decode(float buf[], size_t len, bool& callAgain) override;

        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}

#endif
