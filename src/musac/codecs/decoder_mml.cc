#include <musac/codecs/decoder_mml.hh>
#include <musac/sdk/io_stream.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include <cmath>
#include <algorithm>

namespace musac {

decoder_mml::decoder_mml() = default;
decoder_mml::~decoder_mml() = default;

bool decoder_mml::accept(io_stream* stream) {
    if (!stream) {
        return false;
    }
    
    // Save current stream position
    auto original_pos = stream->tell();
    if (original_pos < 0) {
        return false;
    }
    
    // Read a portion of the file to check for MML commands
    char buffer[512];
    int64_t bytes_read = stream->read(buffer, sizeof(buffer) - 1);
    
    bool result = false;
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        
        // Convert to uppercase for easier matching
        std::string content(buffer);
        std::transform(content.begin(), content.end(), content.begin(), ::toupper);
        
        // Look for common MML commands
        // Note: C, D, E, F, G, A, B notes
        // T (tempo), L (length), O (octave), V (volume), R/P (rest)
        // ML/MN/MS (articulation), < > (octave up/down)
        
        // Check for MML-specific patterns
        bool has_notes = false;
        bool has_commands = false;
        
        // Look for note patterns (letter followed by optional sharp/flat and number)
        for (char note : {'C', 'D', 'E', 'F', 'G', 'A', 'B'}) {
            if (content.find(note) != std::string::npos) {
                has_notes = true;
                break;
            }
        }
        
        // Look for MML commands
        if (content.find('T') != std::string::npos || // Tempo
            content.find('L') != std::string::npos || // Length
            content.find('O') != std::string::npos || // Octave
            content.find('V') != std::string::npos || // Volume
            content.find('R') != std::string::npos || // Rest
            content.find('P') != std::string::npos || // Pause
            content.find('<') != std::string::npos || // Octave down
            content.find('>') != std::string::npos) { // Octave up
            has_commands = true;
        }
        
        // Must have both notes and commands to be considered MML
        // Also check that it's primarily text (not binary)
        bool is_text = true;
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] < 32 && buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != '\t') {
                is_text = false;
                break;
            }
        }
        
        result = is_text && has_notes && has_commands;
    }
    
    // Restore original position
    stream->seek(original_pos, seek_origin::set);
    return result;
}

const char* decoder_mml::get_name() const {
    return "MML (Music Macro Language)";
}

void decoder_mml::open(io_stream* stream) {
    if (!stream) {
        THROW_RUNTIME("No stream provided to MML decoder");
    }
    
    // Read entire MML file into string
    // Since we don't have size(), read until EOF
    std::vector<char> buffer;
    char chunk[1024];
    int64_t bytes_read;
    
    while ((bytes_read = stream->read(chunk, sizeof(chunk))) > 0) {
        buffer.insert(buffer.end(), chunk, chunk + bytes_read);
    }
    
    if (buffer.empty() || buffer.size() > 1024 * 1024) { // Max 1MB for MML files
        THROW_RUNTIME("Invalid MML file size");
    }
    
    m_mml_content.assign(buffer.begin(), buffer.end());
    
    // Parse MML
    parse_mml();
    
    // Convert events to tone queue
    events_to_tones();
    
    set_is_open(true);
    m_current_sample = 0;
    m_current_tone_sample = 0;
    m_phase_accumulator = 0.0f;
}

channels_t decoder_mml::get_channels() const {
    return m_channels;
}

sample_rate_t decoder_mml::get_rate() const {
    return m_sample_rate;
}

std::chrono::microseconds decoder_mml::duration() const {
    if (!is_open()) {
        return std::chrono::microseconds(0);
    }
    
    // Calculate total duration from samples
    auto duration_seconds = static_cast<double>(m_total_samples) / m_sample_rate;
    return std::chrono::microseconds(static_cast<int64_t>(duration_seconds * 1'000'000));
}

bool decoder_mml::rewind() {
    if (!is_open()) {
        return false;
    }
    
    m_current_sample = 0;
    m_current_tone_sample = 0;
    m_phase_accumulator = 0.0f;
    
    // Rebuild tone queue
    events_to_tones();
    
    return true;
}

bool decoder_mml::seek_to_time(std::chrono::microseconds pos) {
    if (!is_open()) {
        return false;
    }
    
    // Convert time to sample position
    auto seconds = pos.count() / 1'000'000.0;
    auto target_sample = static_cast<size_t>(seconds * m_sample_rate);
    
    if (target_sample >= m_total_samples) {
        target_sample = m_total_samples;
    }
    
    // For simplicity, rewind and skip to target
    rewind();
    m_current_sample = target_sample;
    
    // Find the appropriate tone
    m_tone_queue.clear();
    events_to_tones();
    
    while (!m_tone_queue.empty() && 
           m_tone_queue.front().start_sample + m_tone_queue.front().duration_samples <= target_sample) {
        m_tone_queue.pop_front();
    }
    
    if (!m_tone_queue.empty() && m_tone_queue.front().start_sample < target_sample) {
        m_current_tone_sample = target_sample - m_tone_queue.front().start_sample;
    }
    
    return true;
}

std::vector<std::string> decoder_mml::get_warnings() const {
    return m_warnings;
}

size_t decoder_mml::do_decode(float buf[], size_t len, bool& call_again) {
    if (!is_open() || m_tone_queue.empty()) {
        call_again = false;
        return 0;
    }
    
    size_t samples_generated = 0;
    size_t samples_to_generate = len / m_channels;
    
    while (samples_generated < samples_to_generate && !m_tone_queue.empty()) {
        auto& current_tone = m_tone_queue.front();
        size_t remaining_in_tone = current_tone.duration_samples - m_current_tone_sample;
        size_t to_generate = std::min(remaining_in_tone, samples_to_generate - samples_generated);
        
        // Generate samples for this tone
        float* output = buf + (samples_generated * m_channels);
        
        if (current_tone.frequency_hz > 0) {
            generate_square_wave(output, to_generate * m_channels, current_tone.frequency_hz);
        } else {
            generate_silence(output, to_generate * m_channels);
        }
        
        samples_generated += to_generate;
        m_current_tone_sample += to_generate;
        m_current_sample += to_generate;
        
        // Move to next tone if current is finished
        if (m_current_tone_sample >= current_tone.duration_samples) {
            m_tone_queue.pop_front();
            m_current_tone_sample = 0;
            // Don't reset phase accumulator for smoother transitions
        }
    }
    
    call_again = !m_tone_queue.empty();
    return samples_generated * m_channels;
}

void decoder_mml::parse_mml() {
    m_warnings.clear();
    
    try {
        mml_parser parser;
        m_events = parser.parse(m_mml_content);
        m_warnings = parser.get_warnings();
    } catch (const mml_error& e) {
        THROW_RUNTIME("MML parse error: ", e.what());
    }
}

void decoder_mml::events_to_tones() {
    m_tone_queue.clear();
    m_total_samples = 0;
    
    // Convert MML events to tones with articulation
    auto tones = mml_to_tones::convert(m_events);
    
    for (const auto& tone : tones) {
        size_t duration_samples = static_cast<size_t>(
            tone.duration.count() * m_sample_rate / 1000.0
        );
        
        m_tone_queue.push_back({
            tone.frequency_hz,
            duration_samples,
            m_total_samples
        });
        
        m_total_samples += duration_samples;
    }
}

void decoder_mml::generate_square_wave(float* buffer, size_t samples, float frequency) {
    const float period_samples = m_sample_rate / frequency;
    const float amplitude = 0.3f; // Reduced amplitude to prevent clipping
    
    for (size_t i = 0; i < samples; i += m_channels) {
        // Generate square wave
        float value = (std::fmod(m_phase_accumulator, period_samples) < period_samples / 2.0f) 
                      ? amplitude : -amplitude;
        
        // Write to all channels
        for (channels_t ch = 0; ch < m_channels; ++ch) {
            buffer[i + ch] = value;
        }
        
        // Advance phase
        m_phase_accumulator += 1.0f;
        if (m_phase_accumulator >= period_samples) {
            m_phase_accumulator -= period_samples;
        }
    }
}

void decoder_mml::generate_silence(float* buffer, size_t samples) {
    std::fill_n(buffer, samples, 0.0f);
}

} // namespace musac