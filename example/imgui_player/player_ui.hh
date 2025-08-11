#pragma once

#include <functional>
#include <vector>

#include <musac/test_data/loader.hh>

namespace musac::player {

// Forward declarations
class AudioDeviceManager;
class StreamManager;
class WaveformVisualizer;

/**
 * PlayerUI - Handles all UI rendering, separated from business logic
 * 
 * Responsibilities:
 * - Render all UI components
 * - Handle UI state and interactions
 * - Delegate actions to appropriate managers
 * - Keep UI code separate from audio logic
 */
class PlayerUI {
public:
    struct Config {
        bool show_device_section = true;
        bool show_waveform = true;
        bool show_status_bar = true;
        bool show_debug_info = false;
        float window_alpha = 1.0f;
    };
    
    PlayerUI();
    ~PlayerUI() = default;
    
    // Dependencies injection
    void set_device_manager(AudioDeviceManager* mgr) { m_device_manager = mgr; }
    void set_stream_manager(StreamManager* mgr) { m_stream_manager = mgr; }
    void set_waveform_visualizer(WaveformVisualizer* viz) { m_visualizer = viz; }
    
    // Content lists
    void set_music_list(const std::vector<test_data::music_type>& list);
    void set_sound_list(const std::vector<test_data::music_type>& list);
    
    // Main render function
    void render();
    
    // Configuration
    Config& get_config() { return m_config; }
    const Config& get_config() const { return m_config; }
    
    // Callbacks for user actions
    std::function<void()> on_refresh_devices;
    std::function<void(int)> on_switch_device;
    std::function<void(test_data::music_type)> on_play_music;
    std::function<void()> on_stop_music;
    std::function<void(test_data::music_type)> on_play_sound;
    std::function<void()> on_stop_all_sounds;

private:
    // Dependencies
    AudioDeviceManager* m_device_manager = nullptr;
    StreamManager* m_stream_manager = nullptr;
    WaveformVisualizer* m_visualizer = nullptr;
    
    // Configuration
    Config m_config;
    
    // UI State
    int m_selected_music_index = 0;  // Start with first item selected
    int m_selected_sound_index = 0;  // Start with first item selected
    int m_selected_device_index = -1;
    
    // Content lists
    std::vector<test_data::music_type> m_music_list;
    std::vector<test_data::music_type> m_sound_list;
    
    // Render sections
    void render_menu_bar();
    void render_device_section();
    void render_music_section();
    void render_sound_section();
    void render_waveform_section();
    void render_status_bar();
    void render_debug_window();
    
    // UI Helpers
    void render_combo_box(const char* label, 
                         int& selected_index,
                         const std::vector<test_data::music_type>& items);
    void render_device_combo();
    void render_volume_slider(const char* label, float& volume);
    bool render_play_button(const char* label, bool enabled = true);
    bool render_stop_button(const char* label, bool enabled = true);
    
    // Utility
    const char* get_file_type_name(test_data::music_type type) const;
    const char* get_file_type_short_name(test_data::music_type type) const;
};

} // namespace musac::player