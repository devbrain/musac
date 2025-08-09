#ifndef MUSAC_CODECS_DECODER_MML_HH
#define MUSAC_CODECS_DECODER_MML_HH

#include <musac/sdk/decoder.hh>
#include <musac/codecs/export_musac_codecs.h>
#include <musac/sdk/mml_parser.hh>
#include <deque>
#include <memory>
#include <mutex>

namespace musac {

/**
 * MML (Music Macro Language) decoder
 * 
 * Decodes MML text files into square wave audio.
 * MML is a text-based music notation language originally used
 * for PC speaker music in early computers.
 * 
 * Supported commands:
 * - Notes: C, D, E, F, G, A, B (with # or - for sharp/flat)
 * - Rest: R or P
 * - Octave: O0-O6, < (down), > (up)
 * - Tempo: T32-T255 (BPM)
 * - Length: L1-L64 (default note length)
 * - Volume: V0-V15
 * - Articulation: ML (legato), MN (normal), MS (staccato)
 */
class MUSAC_CODECS_EXPORT decoder_mml : public decoder {
public:
    decoder_mml();
    ~decoder_mml() override;
    
    // Decoder interface
    [[nodiscard]] const char* get_name() const override;
    
    void open(io_stream* stream) override;
    [[nodiscard]] channels_t get_channels() const override;
    [[nodiscard]] sample_rate_t get_rate() const override;
    [[nodiscard]] std::chrono::microseconds duration() const override;
    bool rewind() override;
    bool seek_to_time(std::chrono::microseconds pos) override;
    
    // Get last parse warnings (if any)
    [[nodiscard]] std::vector<std::string> get_warnings() const;
    
protected:
    [[nodiscard]] bool do_accept(io_stream* stream) override;
    size_t do_decode(float buf[], size_t len, bool& call_again) override;
    
private:
    struct tone {
        float frequency_hz;
        size_t duration_samples;
        size_t start_sample;
    };
    
    // MML data
    std::string m_mml_content;
    std::vector<mml_event> m_events;
    std::deque<tone> m_tone_queue;
    std::vector<std::string> m_warnings;
    
    // Playback state
    size_t m_current_sample = 0;
    size_t m_total_samples = 0;
    float m_phase_accumulator = 0.0f;
    size_t m_current_tone_sample = 0;
    
    // Audio parameters
    sample_rate_t m_sample_rate = 44100;
    channels_t m_channels = 2;
    
    // Parse MML and populate tone queue
    void parse_mml();
    void events_to_tones();
    
    // Generate square wave
    void generate_square_wave(float* buffer, size_t samples, float frequency);
    void generate_silence(float* buffer, size_t samples);
};

} // namespace musac

#endif // MUSAC_CODECS_DECODER_MML_HH