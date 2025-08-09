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
    
    class MUSAC_EXPORT pc_speaker_stream : public audio_stream {
    public:
        // Constructor
        pc_speaker_stream();
        ~pc_speaker_stream() override;
        
        // Main PC speaker method - queue a tone to play
        void sound(float frequency_hz, std::chrono::milliseconds duration);
        
        // Helper methods
        void beep(float frequency_hz = 1000.0f);  // Short 100ms beep
        void silence(std::chrono::milliseconds duration);  // Queue silence
        void clear_queue();  // Clear all pending sounds
        [[nodiscard]] bool is_queue_empty() const;
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