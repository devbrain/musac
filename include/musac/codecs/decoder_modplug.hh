// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>
#include <musac/codecs/export_musac_codecs.h>
#include <memory>
namespace musac {
    /*!
     * \brief Module music decoder using libmodplug
     * \ingroup codecs
     * 
     * This decoder provides support for tracker module music formats using
     * libmodplug. Module files (MODs) are a family of music file formats
     * originating from the MOD file format on Amiga systems in the late 1980s.
     * 
     * ## Supported Formats
     * - **MOD** - Original ProTracker format (4-31 channels)
     * - **S3M** - ScreamTracker 3 format (up to 32 channels)
     * - **XM**  - FastTracker II Extended Module (up to 32 channels, 128 instruments)
     * - **IT**  - Impulse Tracker format (up to 64 channels, 256 instruments)
     * - **669** - Composer 669 / UNIS 669 format
     * - **AMF** - Advanced Module Format (DSMI)
     * - **AMS** - AMS module format
     * - **DBM** - DigiBooster Pro format
     * - **DMF** - X-Tracker format
     * - **DSM** - DSIK format
     * - **FAR** - Farandole Composer format
     * - **MDL** - DigiTrakker format
     * - **MED** - OctaMED format
     * - **MTM** - MultiTracker format
     * - **OKT** - Oktalyzer format
     * - **PTM** - PolyTracker format
     * - **STM** - ScreamTracker 2 format
     * - **ULT** - UltraTracker format
     * - **UMX** - Unreal Music format
     * - **MT2** - MadTracker 2 format
     * - **PSM** - Protracker Studio format
     * 
     * ## Features
     * - High-quality resampling with configurable quality levels
     * - Support for stereo separation and surround effects
     * - Volume ramping for click-free playback
     * - Loop detection and handling
     * - Pattern jump support
     * - Effects processing (vibrato, tremolo, portamento, etc.)
     * 
     * ## Playback Options
     * The decoder uses default ModPlug settings optimized for quality:
     * - 44100 Hz stereo output
     * - Oversampling enabled for better quality
     * - Noise reduction enabled
     * - Reverb and bass boost can be configured
     * 
     * ## Memory Usage
     * Module files are typically loaded entirely into memory for playback.
     * Memory usage depends on:
     * - Number of samples/instruments
     * - Sample quality (8-bit vs 16-bit)
     * - Module complexity (patterns, channels)
     * 
     * Typical memory usage ranges from 100KB for simple MODs to several MB
     * for complex IT files with high-quality samples.
     * 
     * ## Example Usage
     * \code{.cpp}
     * auto io = musac::io_from_file("music.mod");
     * musac::decoder_modplug decoder;
     * decoder.open(io.get());
     * 
     * // Module files typically have specific channel counts
     * std::cout << "Channels: " << decoder.get_channels() << "\n";
     * std::cout << "Sample rate: " << decoder.get_rate() << " Hz\n";
     * 
     * // Many modules loop indefinitely
     * auto duration = decoder.duration();
     * if (duration.count() == 0) {
     *     std::cout << "Module loops indefinitely\n";
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
     * \note Many tracker modules are designed to loop indefinitely. The duration()
     * method may return 0 for such modules.
     * 
     * \see decoder Base class for all decoders
     * \see decoders_registry For automatic format detection
     */
    class MUSAC_CODECS_EXPORT decoder_modplug : public decoder {
        public:
            decoder_modplug();
            ~decoder_modplug() override;

            /*!
             * \brief Check if a stream contains module music data
             * \param rwops The stream to check
             * \return true if the stream contains a supported module format
             * 
             * This method checks for various module format signatures at the current
             * stream position. The stream position is restored after checking.
             */
            [[nodiscard]] static bool accept(io_stream* rwops);
            
            /*!
             * \brief Get the decoder name
             * \return "ModPlug" string
             */
            [[nodiscard]] const char* get_name() const override;
            
            /*!
             * \brief Open a module file for decoding
             * \param rwops The stream containing module data
             * \throws musac::decoder_error if the stream is not a valid module file
             * 
             * Loads the entire module file into memory and initializes the ModPlug
             * decoder. The stream is read once and can be closed after opening.
             */
            void open(io_stream* rwops) override;
            
            /*!
             * \brief Get the number of audio channels
             * \return Always returns 2 (stereo output)
             * 
             * ModPlug always outputs stereo audio regardless of the module's
             * internal channel count.
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
             * Resets the module to the first pattern/order.
             */
            bool rewind() override;
            
            /*!
             * \brief Get the total duration of the module
             * \return Duration in microseconds, or 0 if the module loops indefinitely
             * 
             * \note Many modules are designed to loop forever and will return 0.
             * Some modules have explicit end markers and will return their actual duration.
             */
            [[nodiscard]] auto duration() const -> std::chrono::microseconds override;
            
            /*!
             * \brief Seek to a specific time position
             * \param pos Target position in microseconds
             * \return true on success, false if seeking is not supported
             * 
             * \note Seeking in module files may not be sample-accurate due to the
             * nature of pattern-based playback and effects processing.
             */
            bool seek_to_time(std::chrono::microseconds pos) override;

        protected:
            size_t do_decode(float* buf, size_t len, bool& callAgain) override;
        private:
            struct impl;
            const std::unique_ptr <impl> m_pimpl;
    };
} // namespace Aulib

/*

Copyright (C) 2014, 2015, 2016, 2017, 2018, 2019 Nikos Chantziaras.

This file is part of SDL_audiolib.

SDL_audiolib is free software: you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

SDL_audiolib is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License
along with SDL_audiolib. If not, see <http://www.gnu.org/licenses/>.

*/
