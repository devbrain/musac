#ifndef MUSAC_MML_PARSER_HH
#define MUSAC_MML_PARSER_HH

#include <musac/sdk/export_musac_sdk.h>
#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include <optional>
#include <stdexcept>

namespace musac {

// MML parsing error with location info
class MUSAC_SDK_EXPORT mml_error : public std::runtime_error {
public:
    mml_error(const std::string& msg, size_t position)
        : std::runtime_error(msg + " at position " + std::to_string(position))
        , m_position(position) {}
    
    size_t position() const { return m_position; }
    
private:
    size_t m_position;
};

// Represents a single musical event from MML
struct MUSAC_SDK_EXPORT mml_event {
    enum class type {
        note,
        rest,
        tempo_change,
        octave_change,
        volume_change,
        articulation_change
    };
    
    type event_type;
    float frequency_hz = 0.0f;  // For notes
    std::chrono::milliseconds duration{0};
    int value = 0;  // For tempo, octave, volume changes
    
    // Helper constructors
    static mml_event make_note(float freq, std::chrono::milliseconds dur) {
        return {type::note, freq, dur, 0};
    }
    
    static mml_event make_rest(std::chrono::milliseconds dur) {
        return {type::rest, 0.0f, dur, 0};
    }
    
    static mml_event make_tempo(int bpm) {
        return {type::tempo_change, 0.0f, std::chrono::milliseconds{0}, bpm};
    }
};

// MML parser with error recovery
class MUSAC_SDK_EXPORT mml_parser {
public:
    mml_parser();
    ~mml_parser();
    
    // Parse MML string and return events
    // Throws mml_error on unrecoverable errors
    // Returns partial results on recoverable errors
    std::vector<mml_event> parse(const std::string& mml);
    
    // Get warnings from last parse (recoverable errors)
    const std::vector<std::string>& get_warnings() const { return m_warnings; }
    
    // Clear warnings
    void clear_warnings() { m_warnings.clear(); }
    
    // Configuration
    void set_strict_mode(bool strict) { m_strict_mode = strict; }
    bool is_strict_mode() const { return m_strict_mode; }
    
private:
    // Parser state
    struct parser_state {
        size_t position = 0;
        int octave = 4;
        int default_length = 4;
        int tempo = 120;
        int volume = 10;
        bool legato = false;
        bool staccato = false;
        std::string input;
    };
    
    // Note frequency table (C4 = middle C)
    static constexpr float note_frequencies[12] = {
        261.63f,  // C
        277.18f,  // C#
        293.66f,  // D
        311.13f,  // D#
        329.63f,  // E
        349.23f,  // F
        369.99f,  // F#
        392.00f,  // G
        415.30f,  // G#
        440.00f,  // A
        466.16f,  // A#
        493.88f   // B
    };
    
    // Parsing methods
    std::optional<mml_event> parse_note(parser_state& state);
    std::optional<mml_event> parse_rest(parser_state& state);
    std::optional<mml_event> parse_octave(parser_state& state);
    std::optional<mml_event> parse_tempo(parser_state& state);
    std::optional<mml_event> parse_length(parser_state& state);
    std::optional<mml_event> parse_volume(parser_state& state);
    std::optional<mml_event> parse_articulation(parser_state& state);
    
    // Helper methods
    int parse_number(parser_state& state, int default_value);
    float get_note_frequency(int note_index, int octave);
    std::chrono::milliseconds note_length_to_duration(int length, int tempo, bool dotted);
    int parse_note_name(char c);
    void skip_whitespace(parser_state& state);
    void add_warning(const std::string& msg, size_t position);
    
    // Error recovery
    void recover_from_error(parser_state& state);
    
    // Configuration
    bool m_strict_mode = false;  // If true, warnings become errors
    
    // Warnings from last parse
    std::vector<std::string> m_warnings;
};

// Helper class to convert MML events to PC speaker tones
class MUSAC_SDK_EXPORT mml_to_tones {
public:
    struct tone {
        float frequency_hz;
        std::chrono::milliseconds duration;
    };
    
    // Convert MML events to simple tones (ignores non-note events)
    static std::vector<tone> convert(const std::vector<mml_event>& events);
    
    // Convert with articulation applied
    static std::vector<tone> convert_with_articulation(
        const std::vector<mml_event>& events,
        float legato_factor = 1.0f,     // 1.0 = full length
        float normal_factor = 0.875f,    // 7/8 length
        float staccato_factor = 0.75f    // 3/4 length
    );
};

} // namespace musac

#endif // MUSAC_MML_PARSER_HH