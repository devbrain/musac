#pragma once

#include <vector>
#include <musac/stream.hh>
#include <musac/audio_device.hh>

namespace musac::player {

/**
 * WaveformVisualizer - Handles audio visualization
 * 
 * Responsibilities:
 * - Fetch audio samples from stream
 * - Process samples for visualization
 * - Render waveform using ImGui
 * - Manage visualization settings
 */
class WaveformVisualizer {
public:
    static constexpr int DEFAULT_SAMPLE_COUNT = 512;
    static constexpr float DEFAULT_HEIGHT = 100.0f;
    
    WaveformVisualizer();
    ~WaveformVisualizer() = default;
    
    // Sample management
    void update_samples(audio_stream* stream);
    void update_from_device(audio_device* device);
    void clear_samples();
    
    // Rendering
    void render(const char* label = "Waveform", 
                float width = -1.0f, 
                float height = DEFAULT_HEIGHT);
    void render_volume_meter(const char* label = "Volume Level");
    
    // Settings
    void set_enabled(bool enabled) { m_enabled = enabled; }
    bool is_enabled() const { return m_enabled; }
    
    void set_sample_count(int count);
    int get_sample_count() const { return m_sample_count; }
    
    void set_color(float r, float g, float b, float a = 1.0f);
    void reset_color();

private:
    std::vector<float> m_waveform_samples;
    std::vector<float> m_temp_buffer;
    
    bool m_enabled = true;
    int m_sample_count = DEFAULT_SAMPLE_COUNT;
    
    // Visualization colors
    struct Color {
        float r = 0.4f, g = 0.7f, b = 0.9f, a = 1.0f;
    } m_color;
    
    // Helper methods
    void fetch_audio_samples(audio_stream* stream);
    void fetch_audio_samples_from_device(audio_device* device);
    void normalize_samples();
    float calculate_rms() const;
};

} // namespace musac::player