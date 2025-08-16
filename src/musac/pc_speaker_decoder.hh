/**
 * @file pc_speaker_decoder.hh
 * @brief Internal PC Speaker decoder for tone generation
 * @ingroup internal
 */

#ifndef MUSAC_PC_SPEAKER_DECODER_HH
#define MUSAC_PC_SPEAKER_DECODER_HH

#include <musac/sdk/decoder.hh>
#include <musac/pc_speaker_stream.hh>
#include <mutex>
#include <deque>

namespace musac {
    
    /*!
     * \brief Internal decoder for PC Speaker tone generation
     * \ingroup internal
     * 
     * This is an internal decoder class used exclusively by pc_speaker_stream
     * to generate square wave audio from a queue of tones. It provides the
     * actual audio synthesis for PC Speaker emulation.
     * 
     * ## Design Purpose
     * This decoder acts as a bridge between the pc_speaker_stream's tone queue
     * and the audio system. It processes tone commands (frequency and duration)
     * and generates the corresponding square wave audio samples.
     * 
     * ## Square Wave Generation
     * The decoder generates authentic PC Speaker-style square waves:
     * - Pure square wave (50% duty cycle)
     * - Frequency range: 37 Hz to 32,767 Hz
     * - Monophonic output (duplicated to stereo)
     * - No amplitude modulation (binary on/off)
     * 
     * ## Tone Queue Processing
     * - Dequeues tones from parent pc_speaker_stream
     * - Processes one tone at a time
     * - Generates silence between tones for articulation
     * - Handles frequency transitions without clicks
     * 
     * ## Real-time Characteristics
     * - Lock-free audio generation in do_decode()
     * - Minimal processing per sample
     * - Predictable performance
     * - No memory allocation during playback
     * 
     * ## Integration with pc_speaker_stream
     * The pc_speaker_stream class:
     * 1. Creates an instance of this decoder
     * 2. Manages the tone queue
     * 3. Uses this decoder as its audio source
     * 4. Handles user-facing API (sound(), beep(), play_mml())
     * 
     * ## Implementation Details
     * - Uses phase accumulator for waveform generation
     * - Maintains phase continuity across frequency changes
     * - Generates samples at 44100 Hz by default
     * - Converts frequency to phase increment for efficiency
     * 
     * \warning This class is internal and should not be used directly.
     * Use pc_speaker_stream for PC Speaker functionality.
     * 
     * \see pc_speaker_stream The public API for PC Speaker emulation
     */
    class pc_speaker_decoder : public decoder {
    public:
        /*!
         * \brief Construct a PC Speaker decoder
         * \param parent The parent pc_speaker_stream that owns this decoder
         * 
         * Initializes the square wave generator and links to the parent
         * stream's tone queue.
         */
        pc_speaker_decoder(pc_speaker_stream* parent);
        ~pc_speaker_decoder() override;
        
        /*!
         * \brief Open the decoder (no-op for PC Speaker)
         * \param stream Ignored parameter (required by interface)
         * 
         * PC Speaker decoder doesn't read from files, so this just
         * marks the decoder as open.
         */
        void open(io_stream* stream) override;
        
        /*!
         * \brief Get the number of audio channels
         * \return Always returns 2 (stereo with duplicated channels)
         */
        [[nodiscard]] channels_t get_channels() const override;
        
        /*!
         * \brief Get the sample rate
         * \return Sample rate in Hz (typically 44100)
         */
        [[nodiscard]] sample_rate_t get_rate() const override;
        
        /*!
         * \brief Get the decoder name
         * \return "PC Speaker" string
         */
        [[nodiscard]] const char* get_name() const override;
        
        /*!
         * \brief Reset playback (clears tone queue)
         * \return Always returns true
         * 
         * Clears any pending tones and resets the generator state.
         */
        bool rewind() override;
        
        /*!
         * \brief Get duration (not applicable for real-time stream)
         * \return Always returns 0 (infinite stream)
         * 
         * PC Speaker is a real-time tone generator with no fixed duration.
         */
        [[nodiscard]] std::chrono::microseconds duration() const override;
        
        /*!
         * \brief Seek to position (not supported)
         * \param pos Ignored parameter
         * \return Always returns false
         * 
         * Seeking is not applicable for real-time tone generation.
         */
        bool seek_to_time(std::chrono::microseconds pos) override;
        
    protected:
        /*!
         * \brief Generate square wave audio samples
         * \param buf Output buffer for generated samples
         * \param len Number of samples to generate
         * \param call_again Set to true if more tones are queued
         * \return Number of samples generated
         * 
         * Core synthesis routine that:
         * 1. Dequeues tones from the parent's tone queue
         * 2. Generates square wave samples at the specified frequency
         * 3. Handles tone transitions and silence periods
         * 4. Duplicates mono output to stereo channels
         * 
         * This method is real-time safe with no allocations or locks.
         */
        size_t do_decode(float* buf, size_t len, bool& call_again) override;
        
    private:
        pc_speaker_stream* m_parent;  ///< Parent stream that owns this decoder
        sample_rate_t m_sample_rate;   ///< Output sample rate (typically 44100)
        
        /// Square wave generator state
        float m_phase;                 ///< Current phase position (0.0 to 1.0)
        float m_phase_increment;       ///< Phase increment per sample
        float m_current_frequency;     ///< Current frequency in Hz
        
        /// Current tone being played
        struct {
            float frequency_hz;         ///< Tone frequency (0 for silence)
            size_t samples_remaining;   ///< Samples left to generate
            bool active;                ///< Whether a tone is currently playing
        } m_current_tone;
        
        /*!
         * \brief Set the frequency for square wave generation
         * \param hz Frequency in Hz (37-32767, or 0 for silence)
         * 
         * Updates the phase increment for the new frequency while
         * maintaining phase continuity to avoid clicks.
         */
        void set_frequency(float hz);
        
        /*!
         * \brief Generate a single square wave sample
         * \return Sample value (-1.0 or 1.0 for square wave, 0.0 for silence)
         * 
         * Advances the phase accumulator and returns the appropriate
         * sample value based on the current phase position.
         */
        float generate_sample();
        
        /*!
         * \brief Dequeue the next tone from the parent's queue
         * \return true if a tone was dequeued, false if queue is empty
         * 
         * Retrieves the next tone command from the parent stream's
         * tone queue and sets up the generator for the new tone.
         */
        bool dequeue_next_tone();
    };
    
} // namespace musac

#endif // MUSAC_PC_SPEAKER_DECODER_HH