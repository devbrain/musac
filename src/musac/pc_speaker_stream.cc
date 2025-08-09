#include <musac/pc_speaker_stream.hh>
#include "pc_speaker_decoder.hh"
#include <musac/audio_system.hh>
#include <musac/sdk/io_stream.hh>
#include <musac/sdk/mml_parser.hh>
#include <iostream>

namespace musac {
    
    pc_speaker_stream::pc_speaker_stream()
        : audio_stream(audio_source(
            std::make_unique<pc_speaker_decoder>(this),
            io_from_memory("DUMMY", 5)  // Small valid io_stream, not used by decoder
          )) {
    }
    
    pc_speaker_stream::~pc_speaker_stream() = default;
    
    void pc_speaker_stream::sound(float frequency_hz, std::chrono::milliseconds duration) {
        // Clamp frequency to reasonable range
        if (frequency_hz < 0.0f) {
            frequency_hz = 0.0f;  // Treat negative as silence
        } else if (frequency_hz > 20000.0f) {
            frequency_hz = 20000.0f;  // Cap at 20kHz
        }
        
        // Don't queue empty durations
        if (duration.count() <= 0) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_tone_queue.push_back({frequency_hz, duration});
    }
    
    void pc_speaker_stream::beep(float frequency_hz) {
        // Standard beep is 100ms
        sound(frequency_hz, std::chrono::milliseconds(100));
    }
    
    void pc_speaker_stream::silence(std::chrono::milliseconds duration) {
        // Queue silence (0 Hz)
        sound(0.0f, duration);
    }
    
    void pc_speaker_stream::clear_queue() {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_tone_queue.clear();
    }
    
    bool pc_speaker_stream::is_queue_empty() const {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        return m_tone_queue.empty();
    }
    
    size_t pc_speaker_stream::queue_size() const {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        return m_tone_queue.size();
    }
    
    bool pc_speaker_stream::play_mml(const std::string& mml, bool strict) {
        try {
            // Clear existing queue
            clear_queue();
            m_last_mml_warnings.clear();
            
            // Parse MML
            mml_parser parser;
            parser.set_strict_mode(strict);
            auto events = parser.parse(mml);
            
            // Store warnings
            m_last_mml_warnings = parser.get_warnings();
            
            // Convert events to tones
            auto tones = mml_to_tones::convert(events);
            
            // Queue all tones
            for (const auto& tone : tones) {
                sound(tone.frequency_hz, tone.duration);
            }
            
            // Start playback
            return audio_stream::play();
            
        } catch (const mml_error& e) {
            // Add error as a warning
            m_last_mml_warnings.push_back(std::string("Parse error: ") + e.what());
            return false;
        } catch (const std::exception& e) {
            m_last_mml_warnings.push_back(std::string("Unexpected error: ") + e.what());
            return false;
        }
    }
    
    std::vector<std::string> pc_speaker_stream::get_mml_warnings() const {
        return m_last_mml_warnings;
    }
    
} // namespace musac