#include "player_ui.hh"
#include "audio_device_manager.hh"
#include "stream_manager.hh"
#include "waveform_visualizer.hh"
#include <musac/test_data/loader.hh>
#include <imgui.h>
#include <sstream>

namespace musac::player {

PlayerUI::PlayerUI() {
}

void PlayerUI::set_music_list(const std::vector<test_data::music_type>& list) {
    m_music_list = list;
    if (m_selected_music_index >= static_cast<int>(list.size())) {
        m_selected_music_index = list.empty() ? -1 : 0;
    }
}

void PlayerUI::set_sound_list(const std::vector<test_data::music_type>& list) {
    m_sound_list = list;
    if (m_selected_sound_index >= static_cast<int>(list.size())) {
        m_selected_sound_index = list.empty() ? -1 : 0;
    }
}

void PlayerUI::render() {
    // Main window
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    
    if (m_config.window_alpha < 1.0f) {
        ImGui::SetNextWindowBgAlpha(m_config.window_alpha);
    }
    
    if (ImGui::Begin("Musac Audio Player")) {
        render_menu_bar();
        
        ImGui::Separator();
        
        if (m_config.show_device_section) {
            render_device_section();
            ImGui::Separator();
        }
        
        render_music_section();
        ImGui::Separator();
        
        render_sound_section();
        
        if (m_config.show_waveform) {
            ImGui::Separator();
            render_waveform_section();
        }
        
        if (m_config.show_status_bar) {
            ImGui::Separator();
            render_status_bar();
        }
    }
    ImGui::End();
    
    if (m_config.show_debug_info) {
        render_debug_window();
    }
}

void PlayerUI::render_menu_bar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Device Section", nullptr, &m_config.show_device_section);
            ImGui::MenuItem("Waveform", nullptr, &m_config.show_waveform);
            ImGui::MenuItem("Status Bar", nullptr, &m_config.show_status_bar);
            ImGui::MenuItem("Debug Info", nullptr, &m_config.show_debug_info);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Settings")) {
            ImGui::SliderFloat("Window Opacity", &m_config.window_alpha, 0.3f, 1.0f);
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void PlayerUI::render_device_section() {
    ImGui::Text("Audio Device:");
    ImGui::Indent();
    
    if (m_device_manager) {
        // Backend info
        ImGui::Text("Backend: %s", m_device_manager->get_backend_name().c_str());
        
        // Device selection
        render_device_combo();
        
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            if (on_refresh_devices) {
                on_refresh_devices();
            }
        }
        
        // Device status
        if (m_device_manager->is_switching()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Switching device...");
        } else if (m_device_manager->is_device_open()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Device Open");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No Device");
        }
    } else {
        ImGui::Text("Device manager not initialized");
    }
    
    ImGui::Unindent();
}

void PlayerUI::render_music_section() {
    ImGui::Text("Background Music:");
    ImGui::Indent();
    
    // Music selection
    render_combo_box("Select Music", m_selected_music_index, m_music_list);
    
    // Controls
    ImGui::SameLine();
    if (render_play_button("Play##Music", m_selected_music_index >= 0)) {
        if (on_play_music && m_selected_music_index >= 0) {
            on_play_music(m_music_list[m_selected_music_index]);
        }
    }
    
    ImGui::SameLine();
    if (render_stop_button("Stop##Music", m_stream_manager && m_stream_manager->is_music_playing())) {
        if (on_stop_music) {
            on_stop_music();
        }
    }
    
    // Volume control
    if (m_stream_manager && m_stream_manager->is_music_playing()) {
        float volume = m_stream_manager->get_music_volume();
        if (ImGui::SliderFloat("Music Volume", &volume, 0.0f, 1.0f, "%.2f")) {
            m_stream_manager->set_music_volume(volume);
        }
        
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Playing");
    }
    
    ImGui::Unindent();
}

void PlayerUI::render_sound_section() {
    ImGui::Text("Sound Effects:");
    ImGui::Indent();
    
    // Sound selection
    render_combo_box("Select Sound", m_selected_sound_index, m_sound_list);
    
    // Controls
    ImGui::SameLine();
    if (render_play_button("Play##Sound", m_selected_sound_index >= 0)) {
        if (on_play_sound && m_selected_sound_index >= 0) {
            on_play_sound(m_sound_list[m_selected_sound_index]);
        }
    }
    
    ImGui::SameLine();
    if (render_stop_button("Stop All##Sounds", 
                          m_stream_manager && m_stream_manager->get_active_sound_count() > 0)) {
        if (on_stop_all_sounds) {
            on_stop_all_sounds();
        }
    }
    
    // Sound volume
    if (m_stream_manager) {
        float volume = m_stream_manager->get_sound_volume();
        if (ImGui::SliderFloat("Sound Volume", &volume, 0.0f, 1.0f, "%.2f")) {
            m_stream_manager->set_sound_volume(volume);
        }
        
        // Active sounds info
        size_t active_sounds = m_stream_manager->get_active_sound_count();
        if (active_sounds > 0) {
            ImGui::SameLine();
            ImGui::Text("Active: %zu", active_sounds);
        }
    }
    
    ImGui::Unindent();
}

void PlayerUI::render_waveform_section() {
    if (!m_visualizer) {
        return;
    }
    
    ImGui::Text("Audio Visualization:");
    ImGui::Indent();
    
    // Update visualization from device to get mixed output
    if (m_device_manager && m_device_manager->get_device()) {
        m_visualizer->update_from_device(m_device_manager->get_device());
    } else if (m_stream_manager) {
        // Fallback to stream visualization if device not available
        m_visualizer->update_samples(m_stream_manager->get_music_stream());
    }
    
    // Render waveform
    m_visualizer->render("Output Waveform", -1.0f, 100.0f);
    
    // Render volume meter
    m_visualizer->render_volume_meter("Volume Level");
    
    ImGui::Unindent();
}

void PlayerUI::render_status_bar() {
    ImGui::Text("Status:");
    ImGui::Indent();
    
    if (m_stream_manager) {
        size_t total_streams = m_stream_manager->get_total_active_streams();
        ImGui::Text("Active Streams: %zu", total_streams);
        
        if (m_stream_manager->is_music_playing()) {
            ImGui::SameLine();
            ImGui::Text("| Music: Playing");
        }
        
        size_t sound_count = m_stream_manager->get_active_sound_count();
        if (sound_count > 0) {
            ImGui::SameLine();
            ImGui::Text("| Sounds: %zu", sound_count);
        }
    }
    
    if (m_device_manager) {
        ImGui::SameLine();
        ImGui::Text("| Device: %s", m_device_manager->get_current_device_name().c_str());
    }
    
    ImGui::Unindent();
}

void PlayerUI::render_debug_window() {
    if (ImGui::Begin("Debug Info", &m_config.show_debug_info)) {
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                   1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        
        if (m_device_manager) {
            ImGui::Separator();
            ImGui::Text("Device Manager:");
            ImGui::Text("  Backend: %s", m_device_manager->get_backend_name().c_str());
            ImGui::Text("  Current Device: %s", m_device_manager->get_current_device_name().c_str());
            ImGui::Text("  Device Open: %s", m_device_manager->is_device_open() ? "Yes" : "No");
        }
        
        if (m_stream_manager) {
            ImGui::Separator();
            ImGui::Text("Stream Manager:");
            ImGui::Text("  Music Playing: %s", m_stream_manager->is_music_playing() ? "Yes" : "No");
            ImGui::Text("  Active Sounds: %zu", m_stream_manager->get_active_sound_count());
            ImGui::Text("  Total Streams: %zu", m_stream_manager->get_total_active_streams());
        }
    }
    ImGui::End();
}

void PlayerUI::render_combo_box(const char* label, 
                                int& selected_index,
                                const std::vector<test_data::music_type>& items) {
    const char* preview = selected_index >= 0 && selected_index < static_cast<int>(items.size()) ?
                         get_file_type_name(items[selected_index]) : "None";
    
    if (ImGui::BeginCombo(label, preview)) {
        for (size_t i = 0; i < items.size(); ++i) {
            bool is_selected = (selected_index == static_cast<int>(i));
            const char* item_name = get_file_type_name(items[i]);
            
            if (ImGui::Selectable(item_name, is_selected)) {
                selected_index = static_cast<int>(i);
            }
            
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

void PlayerUI::render_device_combo() {
    if (!m_device_manager) {
        return;
    }
    
    const auto& devices = m_device_manager->get_device_list();
    m_selected_device_index = m_device_manager->get_current_device_index();
    
    // Store the device name in a persistent string to avoid dangling pointer
    std::string current_device_name = m_device_manager->get_current_device_name();
    
    if (ImGui::BeginCombo("Device", current_device_name.c_str())) {
        for (size_t i = 0; i < devices.size(); ++i) {
            bool is_selected = (m_selected_device_index == static_cast<int>(i));
            std::string display_name = m_device_manager->get_device_display_name(i);
            
            if (ImGui::Selectable(display_name.c_str(), is_selected)) {
                if (on_switch_device) {
                    on_switch_device(static_cast<int>(i));
                }
            }
            
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

bool PlayerUI::render_play_button(const char* label, bool enabled) {
    if (!enabled) {
        ImGui::BeginDisabled();
    }
    
    bool clicked = ImGui::Button(label);
    
    if (!enabled) {
        ImGui::EndDisabled();
    }
    
    return clicked;
}

bool PlayerUI::render_stop_button(const char* label, bool enabled) {
    if (!enabled) {
        ImGui::BeginDisabled();
    }
    
    bool clicked = ImGui::Button(label);
    
    if (!enabled) {
        ImGui::EndDisabled();
    }
    
    return clicked;
}

const char* PlayerUI::get_file_type_name(test_data::music_type type) const {
    return test_data::loader::get_name(type);
}

const char* PlayerUI::get_file_type_short_name(test_data::music_type type) const {
    // Could implement a shorter version for compact UI
    return get_file_type_name(type);
}

} // namespace musac::player