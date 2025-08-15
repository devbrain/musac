/**
 * @file pc_speaker_stream.hh
 * @brief PC speaker emulation and MML support
 * @ingroup pc_speaker
 */

#ifndef MUSAC_PC_SPEAKER_STREAM_HH
#define MUSAC_PC_SPEAKER_STREAM_HH

#include <musac/stream.hh>
#include <musac/export_musac.h>
#include <chrono>
#include <deque>
#include <mutex>
#include <memory>
#include <string>
#include <vector>

namespace musac {
    
    /**
     * @class pc_speaker_stream
     * @brief Emulates classic PC speaker with MML support
     * @ingroup pc_speaker
     * 
     * The pc_speaker_stream class generates square wave tones like classic
     * PC speakers from the 1980s and 1990s. It's perfect for retro game
     * sounds, alert beeps, and playing music using MML (Music Macro Language).
     * 
     * ## Basic Usage - Beeps and Tones
     * 
     * @code
     * auto pc_speaker = device.create_pc_speaker_stream();
     * pc_speaker.play();
     * 
     * // Simple beep
     * pc_speaker.beep();  // 1kHz for 100ms
     * pc_speaker.beep(440.0f);  // A4 note
     * 
     * // Custom tones
     * using namespace std::chrono_literals;
     * pc_speaker.sound(261.63f, 500ms);  // C4 for half second
     * pc_speaker.sound(329.63f, 500ms);  // E4
     * pc_speaker.sound(392.00f, 500ms);  // G4
     * 
     * // Add silence
     * pc_speaker.silence(200ms);
     * @endcode
     * 
     * ## MML (Music Macro Language)
     * 
     * MML is a text-based music notation popular in retro computing:
     * 
     * @code
     * // Play a simple scale
     * pc_speaker.play_mml("T120 L4 C D E F G A B >C");
     * 
     * // Play "Mary Had a Little Lamb"
     * pc_speaker.play_mml(
     *     "T120 L4 "
     *     "E D C D E E E2 "
     *     "D D D2 E G G2 "
     *     "E D C D E E E E "
     *     "D D E D C"
     * );
     * 
     * // With dynamics and articulation
     * pc_speaker.play_mml("T140 V10 ML L8 C E G >C< G E C");
     * @endcode
     * 
     * ## MML Commands
     * 
     * | Command | Description | Example |
     * |---------|-------------|---------|
     * | C D E F G A B | Notes | C4 = quarter note C |
     * | # or + | Sharp | C# or C+ |
     * | - | Flat | B- |
     * | R or P | Rest | R4 = quarter rest |
     * | O0-O6 | Set octave | O4 = middle octave |
     * | < | Octave down | <C = C in lower octave |
     * | > | Octave up | >C = C in higher octave |
     * | T32-T255 | Tempo (BPM) | T120 = 120 BPM |
     * | L1-L64 | Default note length | L4 = quarter note |
     * | V0-V15 | Volume | V10 = medium volume |
     * | ML | Legato | ML = smooth notes |
     * | MN | Normal | MN = normal articulation |
     * | MS | Staccato | MS = short notes |
     * | . | Dotted note | C4. = dotted quarter |
     * | 1-64 | Note length | C8 = eighth note C |
     * 
     * ## Queue Management
     * 
     * Tones are queued and played sequentially:
     * 
     * @code
     * // Queue multiple tones
     * pc_speaker.sound(440.0f, 100ms);
     * pc_speaker.sound(880.0f, 100ms);
     * pc_speaker.sound(440.0f, 100ms);
     * 
     * // Check queue status
     * size_t pending = pc_speaker.queue_size();
     * bool empty = pc_speaker.is_queue_empty();
     * 
     * // Clear pending tones
     * pc_speaker.clear_queue();
     * @endcode
     * 
     * ## Retro Game Examples
     * 
     * @code
     * // Pac-Man death sound
     * void pac_man_death(pc_speaker_stream& pc) {
     *     pc.play_mml("T200 O3 B >F B F <B >F B F");
     * }
     * 
     * // Power-up sound
     * void power_up(pc_speaker_stream& pc) {
     *     for (float freq = 200; freq <= 2000; freq *= 1.1f) {
     *         pc.sound(freq, 10ms);
     *     }
     * }
     * 
     * // Alarm sound
     * void alarm(pc_speaker_stream& pc) {
     *     for (int i = 0; i < 5; ++i) {
     *         pc.sound(800.0f, 100ms);
     *         pc.sound(600.0f, 100ms);
     *     }
     * }
     * @endcode
     * 
     * @note The PC speaker stream inherits from audio_stream, so all
     *       standard stream controls (volume, pause, stop) work normally.
     * 
     * @see audio_stream, audio_device::create_pc_speaker_stream()
     */
    class MUSAC_EXPORT pc_speaker_stream : public audio_stream {
    public:
        /**
         * @brief Constructor
         */
        pc_speaker_stream();
        
        /**
         * @brief Destructor
         */
        ~pc_speaker_stream() override;
        
        /**
         * @brief Queue a tone to play
         * 
         * Adds a tone to the playback queue. Tones play sequentially
         * in the order they were added.
         * 
         * @param frequency_hz Frequency in Hz (0 = silence)
         * @param duration Duration of the tone
         * 
         * @code
         * // Play A4 for 1 second
         * pc_speaker.sound(440.0f, std::chrono::seconds(1));
         * 
         * // Play a chord arpeggio
         * using namespace std::chrono_literals;
         * pc_speaker.sound(261.63f, 100ms);  // C
         * pc_speaker.sound(329.63f, 100ms);  // E
         * pc_speaker.sound(392.00f, 100ms);  // G
         * @endcode
         */
        void sound(float frequency_hz, std::chrono::milliseconds duration);
        
        /**
         * @brief Play a short beep
         * 
         * Convenience method for alert sounds.
         * 
         * @param frequency_hz Frequency in Hz (default 1000 Hz)
         * 
         * @code
         * pc_speaker.beep();       // 1kHz beep
         * pc_speaker.beep(2000.0f); // Higher pitched beep
         * @endcode
         */
        void beep(float frequency_hz = 1000.0f);
        
        /**
         * @brief Queue a period of silence
         * 
         * @param duration Duration of silence
         * 
         * @code
         * pc_speaker.sound(440.0f, 100ms);
         * pc_speaker.silence(50ms);  // Gap between notes
         * pc_speaker.sound(440.0f, 100ms);
         * @endcode
         */
        void silence(std::chrono::milliseconds duration);
        
        /**
         * @brief Clear all pending tones from the queue
         */
        void clear_queue();
        
        /**
         * @brief Check if the tone queue is empty
         * @return true if no tones are queued
         */
        [[nodiscard]] bool is_queue_empty() const;
        
        /**
         * @brief Get the number of queued tones
         * @return Number of pending tones
         */
        [[nodiscard]] size_t queue_size() const;
        
        /**
         * Play MML (Music Macro Language) string
         * @param mml MML string (e.g., "T120 L4 C D E F G A B >C")
         * @param strict If true, warnings become errors (default: false)
         * @return true if MML was parsed successfully, false on error
         * 
         * MML Commands:
         * - Notes: C, D, E, F, G, A, B (with optional # or - for sharp/flat)
         * - Rest: R or P
         * - Octave: O0-O6, < (down), > (up)
         * - Tempo: T32-T255 (BPM)
         * - Length: L1-L64 (note length)
         * - Volume: V0-V15
         * - Articulation: ML (legato), MN (normal), MS (staccato)
         * 
         * Example: "T120 O4 L8 E E F G G F E D C C D E E. D16 D4"
         */
        bool play_mml(const std::string& mml, bool strict = false);
        
        /**
         * Get warnings from last MML parse
         * @return Vector of warning messages (empty if no warnings)
         */
        [[nodiscard]] std::vector<std::string> get_mml_warnings() const;
        
    private:
        friend class pc_speaker_decoder;
        friend class audio_device;
        
        struct tone_command {
            float frequency_hz;  // 0 = silence
            std::chrono::milliseconds duration;
        };
        
        mutable std::mutex m_queue_mutex;
        std::deque<tone_command> m_tone_queue;
        std::vector<std::string> m_last_mml_warnings;
    };
    
} // namespace musac

#endif // MUSAC_PC_SPEAKER_STREAM_HH