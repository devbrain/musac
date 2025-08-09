#include <musac/sdk/mml_parser.hh>
#include <cctype>
#include <cmath>
#include <sstream>
#include <algorithm>

namespace musac {

constexpr float mml_parser::note_frequencies[12];

mml_parser::mml_parser() = default;
mml_parser::~mml_parser() = default;

std::vector<mml_event> mml_parser::parse(const std::string& mml) {
    m_warnings.clear();
    std::vector<mml_event> events;
    
    parser_state state;
    state.input = mml;
    state.position = 0;
    
    while (state.position < mml.length()) {
        skip_whitespace(state);
        if (state.position >= mml.length()) break;
        
        char cmd = std::toupper(mml[state.position]);
        std::optional<mml_event> event;
        
        try {
            switch (cmd) {
                case 'A': case 'B': case 'C': case 'D':
                case 'E': case 'F': case 'G':
                    event = parse_note(state);
                    break;
                    
                case 'R': case 'P':
                    event = parse_rest(state);
                    break;
                    
                case 'O':
                    event = parse_octave(state);
                    break;
                    
                case '<':
                    state.octave = std::max(0, state.octave - 1);
                    state.position++;
                    event = mml_event{mml_event::type::octave_change, 0, std::chrono::milliseconds{0}, state.octave};
                    break;
                    
                case '>':
                    state.octave = std::min(6, state.octave + 1);
                    state.position++;
                    event = mml_event{mml_event::type::octave_change, 0, std::chrono::milliseconds{0}, state.octave};
                    break;
                    
                case 'T':
                    event = parse_tempo(state);
                    break;
                    
                case 'L':
                    event = parse_length(state);
                    break;
                    
                case 'V':
                    event = parse_volume(state);
                    break;
                    
                case 'M':
                    event = parse_articulation(state);
                    break;
                    
                default:
                    // Unknown command - skip and warn
                    add_warning(std::string("Unknown command '") + cmd + "'", state.position);
                    state.position++;
                    if (m_strict_mode) {
                        throw mml_error("Unknown command '" + std::string(1, cmd) + "'", state.position);
                    }
                    break;
            }
            
            if (event.has_value()) {
                events.push_back(event.value());
            }
            
        } catch (const mml_error&) {
            // Re-throw MML errors
            throw;
        } catch (const std::exception& e) {
            // Convert other exceptions to MML errors
            throw mml_error(e.what(), state.position);
        }
    }
    
    return events;
}

std::optional<mml_event> mml_parser::parse_note(parser_state& state) {
    char note_char = state.input[state.position++];
    int note_index = parse_note_name(note_char);
    
    if (note_index < 0) {
        throw mml_error(std::string("Invalid note '") + note_char + "'", state.position - 1);
    }
    
    // Check for sharp/flat
    if (state.position < state.input.length()) {
        char modifier = state.input[state.position];
        if (modifier == '#' || modifier == '+') {
            note_index++;
            state.position++;
        } else if (modifier == '-') {
            note_index--;
            state.position++;
        }
    }
    
    // Normalize note index
    while (note_index < 0) note_index += 12;
    while (note_index >= 12) note_index -= 12;
    
    // Parse length
    int length = parse_number(state, state.default_length);
    
    // Check for dots
    bool dotted = false;
    int dot_count = 0;
    while (state.position < state.input.length() && state.input[state.position] == '.') {
        dotted = true;
        dot_count++;
        state.position++;
        if (dot_count > 2) {
            add_warning("More than 2 dots not supported, ignoring extras", state.position);
            break;
        }
    }
    
    // Calculate frequency and duration
    float frequency = get_note_frequency(note_index, state.octave);
    auto duration = note_length_to_duration(length, state.tempo, dotted);
    
    // Don't apply articulation here - that's for playback
    // The duration in the event should be the full note value
    
    return mml_event::make_note(frequency, duration);
}

std::optional<mml_event> mml_parser::parse_rest(parser_state& state) {
    state.position++; // Skip 'R' or 'P'
    
    int length = parse_number(state, state.default_length);
    
    // Check for dots
    bool dotted = false;
    while (state.position < state.input.length() && state.input[state.position] == '.') {
        dotted = true;
        state.position++;
    }
    
    auto duration = note_length_to_duration(length, state.tempo, dotted);
    return mml_event::make_rest(duration);
}

std::optional<mml_event> mml_parser::parse_octave(parser_state& state) {
    state.position++; // Skip 'O'
    state.octave = parse_number(state, state.octave);
    
    if (state.octave < 0 || state.octave > 6) {
        add_warning("Octave " + std::to_string(state.octave) + " out of range (0-6), clamping", state.position);
        state.octave = std::clamp(state.octave, 0, 6);
    }
    
    return mml_event{mml_event::type::octave_change, 0, std::chrono::milliseconds{0}, state.octave};
}

std::optional<mml_event> mml_parser::parse_tempo(parser_state& state) {
    state.position++; // Skip 'T'
    
    // Check for '=' syntax (variable assignment)
    if (state.position < state.input.length() && state.input[state.position] == '=') {
        state.position++;
        add_warning("Variable assignment (T=) not supported, ignoring", state.position);
        // Skip until next command
        while (state.position < state.input.length() && 
               !std::isalpha(state.input[state.position]) &&
               state.input[state.position] != '<' &&
               state.input[state.position] != '>') {
            state.position++;
        }
        return std::nullopt;
    }
    
    state.tempo = parse_number(state, state.tempo);
    
    if (state.tempo < 32 || state.tempo > 255) {
        add_warning("Tempo " + std::to_string(state.tempo) + " out of range (32-255), clamping", state.position);
        state.tempo = std::clamp(state.tempo, 32, 255);
    }
    
    return mml_event::make_tempo(state.tempo);
}

std::optional<mml_event> mml_parser::parse_length(parser_state& state) {
    state.position++; // Skip 'L'
    state.default_length = parse_number(state, state.default_length);
    
    if (state.default_length < 1 || state.default_length > 64) {
        add_warning("Length " + std::to_string(state.default_length) + " out of range (1-64), using 4", state.position);
        state.default_length = 4;
    }
    
    return std::nullopt; // Length change doesn't produce an event
}

std::optional<mml_event> mml_parser::parse_volume(parser_state& state) {
    state.position++; // Skip 'V'
    state.volume = parse_number(state, state.volume);
    
    if (state.volume < 0 || state.volume > 15) {
        add_warning("Volume " + std::to_string(state.volume) + " out of range (0-15), clamping", state.position);
        state.volume = std::clamp(state.volume, 0, 15);
    }
    
    return mml_event{mml_event::type::volume_change, 0, std::chrono::milliseconds{0}, state.volume};
}

std::optional<mml_event> mml_parser::parse_articulation(parser_state& state) {
    state.position++; // Skip 'M'
    
    if (state.position >= state.input.length()) {
        add_warning("Articulation mode missing after M", state.position);
        return std::nullopt;
    }
    
    char mode = std::toupper(state.input[state.position++]);
    
    switch (mode) {
        case 'L': // Legato
            state.legato = true;
            state.staccato = false;
            break;
        case 'N': // Normal
            state.legato = false;
            state.staccato = false;
            break;
        case 'S': // Staccato
            state.legato = false;
            state.staccato = true;
            break;
        case 'F': // Foreground (ignored)
        case 'B': // Background (ignored)
            add_warning(std::string("M") + mode + " (foreground/background) not supported", state.position);
            break;
        default:
            add_warning(std::string("Unknown articulation mode M") + mode, state.position);
            break;
    }
    
    return mml_event{mml_event::type::articulation_change, 0, std::chrono::milliseconds{0}, 
                     state.legato ? 1 : (state.staccato ? 2 : 0)};
}

int mml_parser::parse_number(parser_state& state, int default_value) {
    int value = 0;
    bool found_digit = false;
    
    while (state.position < state.input.length() && std::isdigit(state.input[state.position])) {
        value = value * 10 + (state.input[state.position] - '0');
        state.position++;
        found_digit = true;
    }
    
    return found_digit ? value : default_value;
}

float mml_parser::get_note_frequency(int note_index, int octave) {
    // Calculate frequency based on octave (C4 = middle C)
    float base_freq = note_frequencies[note_index];
    int octave_diff = octave - 4;
    return base_freq * std::pow(2.0f, octave_diff);
}

std::chrono::milliseconds mml_parser::note_length_to_duration(int length, int tempo, bool dotted) {
    // Calculate duration in milliseconds
    // Quarter note (length=4) at tempo=120 BPM = 500ms
    float beat_duration = 60000.0f / tempo;  // Duration of quarter note in ms
    float note_duration = (4.0f / length) * beat_duration;
    
    if (dotted) {
        note_duration *= 1.5f;
    }
    
    return std::chrono::milliseconds(static_cast<int>(note_duration));
}

int mml_parser::parse_note_name(char c) {
    switch (std::toupper(c)) {
        case 'C': return 0;
        case 'D': return 2;
        case 'E': return 4;
        case 'F': return 5;
        case 'G': return 7;
        case 'A': return 9;
        case 'B': return 11;
        default: return -1;
    }
}

void mml_parser::skip_whitespace(parser_state& state) {
    while (state.position < state.input.length() && 
           std::isspace(state.input[state.position])) {
        state.position++;
    }
}

void mml_parser::add_warning(const std::string& msg, size_t position) {
    std::stringstream ss;
    ss << msg << " at position " << position;
    m_warnings.push_back(ss.str());
}

void mml_parser::recover_from_error(parser_state& state) {
    // Skip to next likely command
    while (state.position < state.input.length()) {
        char c = std::toupper(state.input[state.position]);
        if (std::isalpha(c) || c == '<' || c == '>') {
            break;
        }
        state.position++;
    }
}

// mml_to_tones implementation

std::vector<mml_to_tones::tone> mml_to_tones::convert(const std::vector<mml_event>& events) {
    // Default articulation is normal (7/8 length)
    return convert_with_articulation(events, 1.0f, 0.875f, 0.75f);
}

std::vector<mml_to_tones::tone> mml_to_tones::convert_with_articulation(
    const std::vector<mml_event>& events,
    float legato_factor,
    float normal_factor,
    float staccato_factor) {
    
    std::vector<tone> tones;
    int articulation_mode = 0; // 0=normal, 1=legato, 2=staccato
    
    for (const auto& event : events) {
        switch (event.event_type) {
            case mml_event::type::note: {
                float factor = normal_factor;
                if (articulation_mode == 1) factor = legato_factor;
                else if (articulation_mode == 2) factor = staccato_factor;
                
                auto adjusted_duration = std::chrono::milliseconds(
                    static_cast<int>(event.duration.count() * factor)
                );
                tones.push_back({event.frequency_hz, adjusted_duration});
                
                // Add gap for non-legato
                if (articulation_mode != 1 && factor < 1.0f) {
                    auto gap_duration = std::chrono::milliseconds(
                        static_cast<int>(event.duration.count() * (1.0f - factor))
                    );
                    tones.push_back({0.0f, gap_duration});
                }
                break;
            }
            
            case mml_event::type::rest:
                tones.push_back({0.0f, event.duration});
                break;
                
            case mml_event::type::articulation_change:
                articulation_mode = event.value;
                break;
                
            default:
                break;
        }
    }
    
    return tones;
}

} // namespace musac