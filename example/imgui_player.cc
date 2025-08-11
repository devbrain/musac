#include <iostream>
#include <vector>
#include <memory>
#include <optional>
#include <string>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>

#include <musac/audio_device.hh>
#include <musac/audio_system.hh>
#include <musac/stream.hh>
#include <musac/test_data/loader.hh>
#include <musac/sdk/audio_backend.hh>

#ifdef IMGUI_USE_SDL3
    #include <SDL3/SDL.h>
    #include <backends/imgui_impl_sdl3.h>
    #include <backends/imgui_impl_sdlrenderer3.h>
    #include <musac_backends/sdl3/sdl3_backend.hh>
#else
    #include <SDL2/SDL.h>
    #include <backends/imgui_impl_sdl2.h>
    #include <backends/imgui_impl_sdlrenderer2.h>
    #include <musac_backends/sdl2/sdl2_backend.hh>
#endif

#include <imgui.h>

class ImGuiPlayer {
public:
    ImGuiPlayer() 
        : m_backend(nullptr)
        , m_audio_device(nullptr)
        , m_selected_device(-1) {
        // Initialize test data
        musac::test_data::loader::init();
        
        // Create appropriate backend
#ifdef MUSAC_HAS_SDL3_BACKEND
        m_backend = musac::create_sdl3_backend();
#elif MUSAC_HAS_SDL2_BACKEND
        m_backend = musac::create_sdl2_backend();
#else
        #error "No SDL backend available"
#endif
        
        // Initialize audio system
        musac::audio_system::init(m_backend);
        
        // Enumerate available devices
        refresh_device_list();
        
        // Open default audio device
        if (!m_devices.empty()) {
            // Find the default device
            for (size_t i = 0; i < m_devices.size(); ++i) {
                if (m_devices[i].is_default) {
                    m_selected_device = static_cast<int>(i);
                    break;
                }
            }
            if (m_selected_device == -1) {
                m_selected_device = 0; // Use first device if no default found
            }
            
            open_selected_device();
        }
        
        // Prepare lists of music and sound effects
        // Manually iterate through all music_type enum values
        const std::vector<musac::test_data::music_type> all_types = {
            musac::test_data::music_type::cmf,
            musac::test_data::music_type::hmp,
            musac::test_data::music_type::mid,
            musac::test_data::music_type::mml_bouree,
            musac::test_data::music_type::mml_complex,
            musac::test_data::music_type::mp3,
            musac::test_data::music_type::opb,
            musac::test_data::music_type::s3m,
            musac::test_data::music_type::voc,
            musac::test_data::music_type::xmi,
            musac::test_data::music_type::vorbis
        };
        
        for (auto type : all_types) {
            if (musac::test_data::loader::is_music(type)) {
                m_music_types.push_back(type);
            } else {
                m_sound_types.push_back(type);
            }
        }
    }
    
    ~ImGuiPlayer() {
        // Clean up
        musac::audio_system::done();
        musac::test_data::loader::done();
    }
    
    void render() {
        ImGui::Begin("Musac Audio Mixer Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::Text("This demo shows how Musac can mix multiple audio streams");
        ImGui::Text("You can play music and sound effects simultaneously");
        ImGui::Separator();
        
        // Backend information
        if (m_backend) {
            ImGui::Text("Audio Backend: %s", m_backend->get_name().c_str());
            ImGui::SameLine();
            ImGui::TextColored(
                m_backend->is_initialized() ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                m_backend->is_initialized() ? "[Initialized]" : "[Not Initialized]"
            );
            
            // Additional backend info
            ImGui::Text("Supports Recording: %s", m_backend->supports_recording() ? "Yes" : "No");
            ImGui::Text("Max Open Devices: %d", m_backend->get_max_open_devices());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No Audio Backend Available");
        }
        ImGui::Separator();
        
        // Audio device selection
        ImGui::Text("Audio Device:");
        ImGui::Indent();
        
        // Device selection combo box
        const char* current_device_name = (m_selected_device >= 0 && m_selected_device < m_devices.size()) ?
            m_devices[m_selected_device].name.c_str() : "None";
        
        if (ImGui::BeginCombo("Select Device", current_device_name)) {
            for (size_t i = 0; i < m_devices.size(); ++i) {
                bool is_selected = (m_selected_device == static_cast<int>(i));
                std::string device_label = m_devices[i].name;
                if (m_devices[i].is_default) {
                    device_label += " (Default)";
                }
                
                if (ImGui::Selectable(device_label.c_str(), is_selected)) {
                    if (m_selected_device != static_cast<int>(i)) {
                        m_selected_device = static_cast<int>(i);
                        switch_device();
                    }
                }
                
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Refresh Devices")) {
            refresh_device_list();
        }
        
        // Show device info
        if (m_audio_device) {
            ImGui::Text("Sample Rate: %d Hz, Channels: %d", 
                       m_audio_device->get_freq(), 
                       m_audio_device->get_channels());
        }
        
        ImGui::Unindent();
        ImGui::Separator();
        
        // Music section
        ImGui::Text("Background Music:");
        ImGui::Indent();
        
        // Music selection combo box
        const char* current_music_name = m_selected_music >= 0 ? 
            musac::test_data::loader::get_name(m_music_types[m_selected_music]) : "None";
        
        if (ImGui::BeginCombo("Select Music", current_music_name)) {
            for (size_t i = 0; i < m_music_types.size(); i++) {
                bool is_selected = (m_selected_music == static_cast<int>(i));
                const char* music_name = musac::test_data::loader::get_name(m_music_types[i]);
                
                if (ImGui::Selectable(music_name, is_selected)) {
                    m_selected_music = i;
                }
                
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        // Music controls
        ImGui::SameLine();
        if (ImGui::Button("Play Music")) {
            play_music();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Stop Music")) {
            stop_music();
        }
        
        // Music volume
        if (m_music_stream && m_music_stream->is_playing()) {
            float volume = m_music_volume;
            if (ImGui::SliderFloat("Music Volume", &volume, 0.0f, 1.0f, "%.2f")) {
                m_music_volume = volume;
                m_music_stream->set_volume(volume);
            }
            
            // Show playing status
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Playing");
        }
        
        ImGui::Unindent();
        ImGui::Separator();
        
        // Sound effects section
        ImGui::Text("Sound Effects:");
        ImGui::Indent();
        
        // Sound selection combo box
        const char* current_sound_name = m_selected_sound >= 0 ? 
            musac::test_data::loader::get_name(m_sound_types[m_selected_sound]) : "None";
        
        if (ImGui::BeginCombo("Select Sound", current_sound_name)) {
            for (size_t i = 0; i < m_sound_types.size(); i++) {
                bool is_selected = (m_selected_sound == static_cast<int>(i));
                const char* sound_name = musac::test_data::loader::get_name(m_sound_types[i]);
                
                if (ImGui::Selectable(sound_name, is_selected)) {
                    m_selected_sound = i;
                }
                
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        // Sound controls
        ImGui::SameLine();
        if (ImGui::Button("Play Sound")) {
            play_sound();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Stop All Sounds")) {
            stop_all_sounds();
        }
        
        // Global sound volume
        if (ImGui::SliderFloat("Sound Volume", &m_sound_volume, 0.0f, 1.0f, "%.2f")) {
            // Apply to all active sound streams
            for (auto& stream : m_sound_streams) {
                if (stream) {
                    stream->set_volume(m_sound_volume);
                }
            }
        }
        
        // Show active sounds count
        int active_sounds = 0;
        for (const auto& stream : m_sound_streams) {
            if (stream && stream->is_playing()) {
                active_sounds++;
            }
        }
        if (active_sounds > 0) {
            ImGui::Text("Active Sounds: %d", active_sounds);
        }
        
        ImGui::Unindent();
        ImGui::Separator();
        
        // Mixing info
        ImGui::Text("Mixing Information:");
        ImGui::Indent();
        
        int total_streams = (m_music_stream && m_music_stream->is_playing() ? 1 : 0) + active_sounds;
        ImGui::Text("Total Active Streams: %d", total_streams);
        
        ImGui::Unindent();
        ImGui::Separator();
        
        // Instructions
        if (ImGui::CollapsingHeader("Instructions")) {
            ImGui::TextWrapped("1. Select a music track from the dropdown and click 'Play Music'");
            ImGui::TextWrapped("2. Select a sound effect and click 'Play Sound' (can be clicked multiple times)");
            ImGui::TextWrapped("3. Adjust volumes to hear how Musac mixes the audio streams");
            ImGui::TextWrapped("4. Notice how multiple sounds can play simultaneously with the music");
        }
        
        ImGui::Separator();
        
        // Waveform visualization
        if (ImGui::CollapsingHeader("Waveform Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto samples = m_audio_device->get_output_buffer();
            if (!samples.empty()) {
                // Downsample for smooth display (show 512 points)
                const int display_samples = 512;
                std::vector<float> display_buffer(display_samples);
                
                // Calculate step size for downsampling
                float step = samples.size() / float(display_samples);
                
                // Downsample the buffer
                for (int i = 0; i < display_samples; ++i) {
                    int idx = int(i * step);
                    if (idx < samples.size()) {
                        display_buffer[i] = samples[idx];
                    }
                }
                
                // Display the waveform
                ImGui::PlotLines("##Waveform", display_buffer.data(), display_samples, 
                                0, "Output Waveform", -1.0f, 1.0f, ImVec2(0, 100));
                
                // Calculate and display RMS (volume level)
                float rms = 0.0f;
                for (float sample : display_buffer) {
                    rms += sample * sample;
                }
                rms = std::sqrt(rms / display_buffer.size());
                
                ImGui::ProgressBar(rms, ImVec2(0, 0), "Volume Level");
            } else {
                ImGui::Text("No audio data available");
            }
        }
        
        ImGui::End();
        
        // Clean up finished sound streams
        cleanup_finished_sounds();
    }
    
private:
    void play_music() {
        if (m_selected_music < 0 || m_selected_music >= static_cast<int>(m_music_types.size())) {
            return;
        }
        
        // Stop current music if playing
        stop_music();
        
        // Load and play new music
        auto source = musac::test_data::loader::load(m_music_types[m_selected_music]);
        auto stream = m_audio_device->create_stream(std::move(source));
        m_music_stream = std::make_unique<musac::audio_stream>(std::move(stream));
        m_music_stream->set_volume(m_music_volume);
        m_music_stream->open();
        m_music_stream->play();
    }
    
    void stop_music() {
        if (m_music_stream) {
            m_music_stream->stop();
            m_music_stream.reset();
        }
    }
    
    void play_sound() {
        if (m_selected_sound < 0 || m_selected_sound >= static_cast<int>(m_sound_types.size())) {
            return;
        }
        
        // Create a new stream for this sound effect
        auto source = musac::test_data::loader::load(m_sound_types[m_selected_sound]);
        auto stream = m_audio_device->create_stream(std::move(source));
        auto stream_ptr = std::make_unique<musac::audio_stream>(std::move(stream));
        stream_ptr->set_volume(m_sound_volume);
        stream_ptr->open();
        stream_ptr->play();
        
        m_sound_streams.push_back(std::move(stream_ptr));
    }
    
    void stop_all_sounds() {
        for (auto& stream : m_sound_streams) {
            if (stream) {
                stream->stop();
            }
        }
        m_sound_streams.clear();
    }
    
    void cleanup_finished_sounds() {
        // Remove sound streams that have finished playing
        m_sound_streams.erase(
            std::remove_if(m_sound_streams.begin(), m_sound_streams.end(),
                [](const std::unique_ptr<musac::audio_stream>& stream) {
                    return !stream || !stream->is_playing();
                }),
            m_sound_streams.end()
        );
    }
    
    void refresh_device_list() {
        m_devices = musac::audio_device::enumerate_devices(m_backend, true);
    }
    
    void open_selected_device() {
        if (m_selected_device < 0 || m_selected_device >= static_cast<int>(m_devices.size())) {
            return;
        }
        
        // Stop all streams before switching
        stop_music();
        stop_all_sounds();
        
        // Open the selected device
        m_audio_device = std::make_unique<musac::audio_device>(
            musac::audio_device::open_device(m_backend, m_devices[m_selected_device].id));
        m_audio_device->set_gain(1.0f);
        m_audio_device->resume();
    }
    
    void switch_device() {
        // Stop all audio before switching
        stop_music();
        stop_all_sounds();
        
        // Give audio system time to stop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Open the new device
        open_selected_device();
    }
    
    std::shared_ptr<musac::audio_backend> m_backend;
    std::unique_ptr<musac::audio_device> m_audio_device;
    std::vector<musac::device_info> m_devices;
    int m_selected_device;
    
    std::vector<musac::test_data::music_type> m_music_types;
    std::vector<musac::test_data::music_type> m_sound_types;
    
    int m_selected_music = 0;
    int m_selected_sound = 0;
    float m_music_volume = 0.7f;
    float m_sound_volume = 1.0f;
    
    std::unique_ptr<musac::audio_stream> m_music_stream;
    std::vector<std::unique_ptr<musac::audio_stream>> m_sound_streams;
};

int main(int argc, char* argv[]) {
    // Initialize SDL
#ifdef IMGUI_USE_SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cerr << "SDL3 initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
#else
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL2 initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
#endif
    
    // Create window
#ifdef IMGUI_USE_SDL3
    SDL_Window* window = SDL_CreateWindow("Musac ImGui Audio Player",
                                          800, 600,
                                          SDL_WINDOW_RESIZABLE);
#else
    SDL_Window* window = SDL_CreateWindow("Musac ImGui Audio Player",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          800, 600,
                                          SDL_WINDOW_RESIZABLE);
#endif
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
#ifdef IMGUI_USE_SDL3
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
#else
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
#endif
    
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
#ifdef IMGUI_USE_SDL3
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
#else
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
#endif
    
    // Create our audio player
    ImGuiPlayer player;
    
    // Main loop
    bool done = false;
    while (!done) {
        // Poll events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
#ifdef IMGUI_USE_SDL3
            ImGui_ImplSDL3_ProcessEvent(&event);
#else
            ImGui_ImplSDL2_ProcessEvent(&event);
#endif
#ifdef IMGUI_USE_SDL3
            if (event.type == SDL_EVENT_QUIT) {
                done = true;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    done = true;
                }
            }
#else
            if (event.type == SDL_QUIT) {
                done = true;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    done = true;
                }
            }
#endif
        }
        
        // Start the ImGui frame
#ifdef IMGUI_USE_SDL3
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
#else
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
#endif
        ImGui::NewFrame();
        
        // Render our player UI
        player.render();
        
        // Rendering
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 45, 45, 50, 255);
        SDL_RenderClear(renderer);
#ifdef IMGUI_USE_SDL3
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
#else
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
#endif
        SDL_RenderPresent(renderer);
    }
    
    // Cleanup
#ifdef IMGUI_USE_SDL3
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
#else
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
#endif
    ImGui::DestroyContext();
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}