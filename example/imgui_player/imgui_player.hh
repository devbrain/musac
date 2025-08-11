#pragma once

#include <memory>
#include <vector>

#include <musac/test_data/loader.hh>

// Forward declarations
struct SDL_Window;
struct SDL_Renderer;

namespace musac {
class audio_backend;
}

namespace musac::player {

// Forward declarations
class AudioDeviceManager;
class StreamManager;
class WaveformVisualizer;
class PlayerUI;

/**
 * ImGuiPlayer - Main application class
 * 
 * This class coordinates all components and manages the application lifecycle.
 * It follows the Single Responsibility Principle by delegating specific tasks
 * to specialized components.
 */
class ImGuiPlayer {
public:
    ImGuiPlayer();
    ~ImGuiPlayer();
    
    // Application lifecycle
    bool init();
    void run();
    void shutdown();
    
private:
    // Components (each with single responsibility)
    std::unique_ptr<AudioDeviceManager> m_device_manager;
    std::unique_ptr<StreamManager> m_stream_manager;
    std::unique_ptr<WaveformVisualizer> m_waveform_visualizer;
    std::unique_ptr<PlayerUI> m_ui;
    
    // SDL context
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    
    // Backend
    std::shared_ptr<audio_backend> m_backend;
    
    // Content lists
    std::vector<test_data::music_type> m_music_types;
    std::vector<test_data::music_type> m_sound_types;
    
    // Application state
    bool m_running = true;
    bool m_initialized = false;
    
    // Initialization helpers
    bool init_sdl();
    bool init_imgui();
    bool init_audio();
    void init_content_lists();
    void connect_components();
    
    // Main loop helpers
    void process_events();
    void update();
    void render();
    
    // Cleanup
    void cleanup_imgui();
    void cleanup_sdl();
    
    // Event handlers (connected to UI callbacks)
    void handle_refresh_devices();
    void handle_switch_device(int device_index);
    void handle_play_music(test_data::music_type type);
    void handle_stop_music();
    void handle_play_sound(test_data::music_type type);
    void handle_stop_all_sounds();
};

} // namespace musac::player