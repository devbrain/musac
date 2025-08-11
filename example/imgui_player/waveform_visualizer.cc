#include "waveform_visualizer.hh"
#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <musac/audio_device.hh>

namespace musac::player {

WaveformVisualizer::WaveformVisualizer() {
    m_waveform_samples.resize(m_sample_count, 0.0f);
    m_temp_buffer.reserve(m_sample_count * 2); // Reserve for stereo
}

void WaveformVisualizer::update_samples(audio_stream* stream) {
    if (!m_enabled || !stream || !stream->is_playing()) {
        clear_samples();
        return;
    }
    
    fetch_audio_samples(stream);
    normalize_samples();
}

void WaveformVisualizer::update_from_device(audio_device* device) {
    if (!m_enabled || !device) {
        clear_samples();
        return;
    }
    
    fetch_audio_samples_from_device(device);
    normalize_samples();
}

void WaveformVisualizer::clear_samples() {
    std::fill(m_waveform_samples.begin(), m_waveform_samples.end(), 0.0f);
}

void WaveformVisualizer::render(const char* label, float width, float height) {
    if (!m_enabled) {
        return;
    }
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImVec2(width < 0 ? ImGui::GetContentRegionAvail().x : width, height);
    
    // Draw border
    draw_list->AddRect(canvas_pos, 
                      ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                      IM_COL32(100, 100, 100, 255));
    
    // Draw center line
    float center_y = canvas_pos.y + canvas_size.y * 0.5f;
    draw_list->AddLine(ImVec2(canvas_pos.x, center_y),
                       ImVec2(canvas_pos.x + canvas_size.x, center_y),
                       IM_COL32(60, 60, 60, 255));
    
    // Draw waveform
    if (!m_waveform_samples.empty()) {
        const float sample_width = canvas_size.x / m_waveform_samples.size();
        const float half_height = canvas_size.y * 0.5f;
        
        ImU32 color = IM_COL32(
            static_cast<int>(m_color.r * 255),
            static_cast<int>(m_color.g * 255),
            static_cast<int>(m_color.b * 255),
            static_cast<int>(m_color.a * 255)
        );
        
        for (size_t i = 1; i < m_waveform_samples.size(); ++i) {
            float x1 = canvas_pos.x + (i - 1) * sample_width;
            float x2 = canvas_pos.x + i * sample_width;
            float y1 = center_y - m_waveform_samples[i - 1] * half_height;
            float y2 = center_y - m_waveform_samples[i] * half_height;
            
            draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, 1.5f);
        }
    }
    
    // Calculate RMS for level display
    float rms = calculate_rms();
    
    // Draw RMS level indicator
    if (rms > 0.01f) {
        float rms_y_top = center_y - rms * canvas_size.y * 0.5f;
        float rms_y_bottom = center_y + rms * canvas_size.y * 0.5f;
        
        ImU32 rms_color = IM_COL32(255, 200, 100, 100);
        draw_list->AddRectFilled(
            ImVec2(canvas_pos.x, rms_y_top),
            ImVec2(canvas_pos.x + canvas_size.x, rms_y_bottom),
            rms_color
        );
    }
    
    // Reserve space in layout
    ImGui::Dummy(canvas_size);
    
    // Add label
    if (label && strlen(label) > 0) {
        ImGui::Text("%s (RMS: %.2f)", label, rms);
    }
}

void WaveformVisualizer::set_sample_count(int count) {
    if (count > 0 && count != m_sample_count) {
        m_sample_count = count;
        m_waveform_samples.resize(m_sample_count, 0.0f);
        m_temp_buffer.reserve(m_sample_count * 2);
    }
}

void WaveformVisualizer::set_color(float r, float g, float b, float a) {
    m_color.r = std::clamp(r, 0.0f, 1.0f);
    m_color.g = std::clamp(g, 0.0f, 1.0f);
    m_color.b = std::clamp(b, 0.0f, 1.0f);
    m_color.a = std::clamp(a, 0.0f, 1.0f);
}

void WaveformVisualizer::reset_color() {
    m_color = Color{0.4f, 0.7f, 0.9f, 1.0f};
}

void WaveformVisualizer::fetch_audio_samples(audio_stream* stream) {
    if (!stream) {
        return;
    }
    
    // For now, generate a simple sine wave visualization
    // TODO: In a real implementation, fetch actual samples from the stream
    static float phase = 0.0f;
    const float frequency = 0.02f;
    
    for (int i = 0; i < m_sample_count; ++i) {
        m_waveform_samples[i] = std::sin(phase + i * frequency) * 0.5f;
    }
    
    phase += frequency * m_sample_count;
    if (phase > 2 * M_PI) {
        phase -= 2 * M_PI;
    }
}

void WaveformVisualizer::fetch_audio_samples_from_device(audio_device* device) {
    if (!device) {
        return;
    }
    
    // Get the output buffer from the device
    auto samples = device->get_output_buffer();
    if (samples.empty()) {
        clear_samples();
        return;
    }
    
    // Downsample or upsample to match our display sample count
    float step = static_cast<float>(samples.size()) / static_cast<float>(m_sample_count);
    
    for (int i = 0; i < m_sample_count; ++i) {
        int idx = static_cast<int>(i * step);
        if (idx < static_cast<int>(samples.size())) {
            m_waveform_samples[i] = samples[idx];
        } else {
            m_waveform_samples[i] = 0.0f;
        }
    }
}

void WaveformVisualizer::normalize_samples() {
    // Find peak value
    float peak = 0.0f;
    for (float sample : m_waveform_samples) {
        peak = std::max(peak, std::abs(sample));
    }
    
    // Normalize if peak is significant
    if (peak > 0.0f && peak != 1.0f) {
        float scale = 1.0f / peak;
        for (float& sample : m_waveform_samples) {
            sample *= scale;
        }
    }
}

float WaveformVisualizer::calculate_rms() const {
    if (m_waveform_samples.empty()) {
        return 0.0f;
    }
    
    float sum_squares = 0.0f;
    for (float sample : m_waveform_samples) {
        sum_squares += sample * sample;
    }
    
    return std::sqrt(sum_squares / m_waveform_samples.size());
}

void WaveformVisualizer::render_volume_meter(const char* label) {
    if (!m_enabled) {
        return;
    }
    
    float rms = calculate_rms();
    
    // Display as a progress bar
    ImGui::ProgressBar(rms, ImVec2(0, 0), label);
    
    // Show numeric value
    ImGui::SameLine();
    ImGui::Text("RMS: %.3f", rms);
}

} // namespace musac::player