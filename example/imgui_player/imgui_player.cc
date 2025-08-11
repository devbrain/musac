#include "imgui_player.hh"
#include "audio_device_manager.hh"
#include "stream_manager.hh"
#include "waveform_visualizer.hh"
#include "player_ui.hh"

#include <musac/audio_system.hh>
#include <musac/test_data/loader.hh>

// Backend includes
#ifdef MUSAC_HAS_SDL3_BACKEND
    #include <musac_backends/sdl3/sdl3_backend.hh>
#elif defined(MUSAC_HAS_SDL2_BACKEND)
    #include <musac_backends/sdl2/sdl2_backend.hh>
#endif

#include <imgui.h>

// Handle SDL2 vs SDL3 includes
#ifdef IMGUI_USE_SDL3
    #include <imgui_impl_sdl3.h>
    #include <imgui_impl_sdlrenderer3.h>
    #include <SDL3/SDL.h>
#else
    #include <imgui_impl_sdl2.h>
    #include <imgui_impl_sdlrenderer2.h>
    #include <SDL.h>
#endif

#include <iostream>
#include <thread>

namespace musac::player {

ImGuiPlayer::ImGuiPlayer() {
    // Components will be created in init()
}

ImGuiPlayer::~ImGuiPlayer() {
    if (m_initialized) {
        shutdown();
    }
}

bool ImGuiPlayer::init() {
    if (m_initialized) {
        return true;
    }
    
    // Initialize SDL
    if (!init_sdl()) {
        return false;
    }
    
    // Initialize ImGui
    if (!init_imgui()) {
        cleanup_sdl();
        return false;
    }
    
    // Initialize audio
    if (!init_audio()) {
        cleanup_imgui();
        cleanup_sdl();
        return false;
    }
    
    // Initialize content lists
    init_content_lists();
    
    // Connect components
    connect_components();
    
    m_initialized = true;
    return true;
}

void ImGuiPlayer::run() {
    if (!m_initialized) {
        return;
    }
    
    while (m_running) {
        process_events();
        update();
        render();
    }
}

void ImGuiPlayer::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    // Stop all audio
    if (m_stream_manager) {
        m_stream_manager->stop_all_streams();
    }
    
    // Cleanup components
    m_ui.reset();
    m_waveform_visualizer.reset();
    m_stream_manager.reset();
    m_device_manager.reset();
    
    // Cleanup audio system
    audio_system::done();
    test_data::loader::done();
    
    // Cleanup ImGui and SDL
    cleanup_imgui();
    cleanup_sdl();
    
    m_initialized = false;
}

bool ImGuiPlayer::init_sdl() {
    // Initialize SDL
#ifdef IMGUI_USE_SDL3
    // SDL3 returns SDL_TRUE (non-zero) on success
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
#else
    // SDL2 returns 0 on success
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
#endif
        std::cerr << "Error: SDL_Init: " << SDL_GetError() << "\n";
        return false;
    }
    
    // Create window
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_RESIZABLE
#ifndef IMGUI_USE_SDL3
        | SDL_WINDOW_ALLOW_HIGHDPI
#else
        | SDL_WINDOW_HIGH_PIXEL_DENSITY
#endif
    );
    
#ifdef IMGUI_USE_SDL3
    m_window = SDL_CreateWindow(
        "Musac ImGui Player",
        1280, 720,
        window_flags
    );
#else
    m_window = SDL_CreateWindow(
        "Musac ImGui Player",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        window_flags
    );
#endif
    
    if (!m_window) {
        std::cerr << "Error: SDL_CreateWindow: " << SDL_GetError() << "\n";
        return false;
    }
    
    // Create renderer
#ifdef IMGUI_USE_SDL3
    m_renderer = SDL_CreateRenderer(m_window, nullptr);
#else
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
#endif
    
    if (!m_renderer) {
        std::cerr << "Error: SDL_CreateRenderer: " << SDL_GetError() << "\n";
        return false;
    }
    
    return true;
}

bool ImGuiPlayer::init_imgui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
#ifdef IMGUI_USE_SDL3
    ImGui_ImplSDL3_InitForOther(m_window);
    ImGui_ImplSDLRenderer3_Init(m_renderer);
#else
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer2_Init(m_renderer);
#endif
    
    return true;
}

bool ImGuiPlayer::init_audio() {
    try {
        // Initialize test data loader
        test_data::loader::init();
        
        // Create backend
#ifdef MUSAC_HAS_SDL3_BACKEND
        m_backend = create_sdl3_backend();
#elif defined(MUSAC_HAS_SDL2_BACKEND)
        m_backend = create_sdl2_backend();
#else
        #error "No SDL backend available"
#endif
        
        if (!m_backend) {
            std::cerr << "Failed to create audio backend\n";
            return false;
        }
        
        // Initialize audio system with backend
        if (!audio_system::init(m_backend)) {
            std::cerr << "Failed to initialize audio system\n";
            return false;
        }
        
        // Create components
        m_device_manager = std::make_unique<AudioDeviceManager>();
        m_device_manager->init(m_backend);
        
        m_stream_manager = std::make_unique<StreamManager>(m_device_manager.get());
        m_waveform_visualizer = std::make_unique<WaveformVisualizer>();
        m_ui = std::make_unique<PlayerUI>();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Audio initialization failed: " << e.what() << "\n";
        return false;
    }
}

void ImGuiPlayer::init_content_lists() {
    // Get all available music types
    std::vector<test_data::music_type> all_types = {
        test_data::music_type::cmf,
        test_data::music_type::hmp,
        test_data::music_type::mid,
        test_data::music_type::mml_bouree,
        test_data::music_type::mml_complex,
        test_data::music_type::mp3,
        test_data::music_type::opb,
        test_data::music_type::s3m,
        test_data::music_type::voc,
        test_data::music_type::xmi,
        test_data::music_type::vorbis
    };
    
    // Separate into music and sound effects based on type
    for (auto type : all_types) {
        if (test_data::loader::is_music(type)) {
            m_music_types.push_back(type);
        } else {
            m_sound_types.push_back(type);
        }
    }
}

void ImGuiPlayer::connect_components() {
    // Set UI dependencies
    m_ui->set_device_manager(m_device_manager.get());
    m_ui->set_stream_manager(m_stream_manager.get());
    m_ui->set_waveform_visualizer(m_waveform_visualizer.get());
    
    // Set content lists
    m_ui->set_music_list(m_music_types);
    m_ui->set_sound_list(m_sound_types);
    
    // Connect UI callbacks to handlers
    m_ui->on_refresh_devices = [this]() { 
        handle_refresh_devices(); 
    };
    
    m_ui->on_switch_device = [this](int index) { 
        handle_switch_device(index); 
    };
    
    m_ui->on_play_music = [this](test_data::music_type type) { 
        handle_play_music(type); 
    };
    
    m_ui->on_stop_music = [this]() { 
        handle_stop_music(); 
    };
    
    m_ui->on_play_sound = [this](test_data::music_type type) { 
        handle_play_sound(type); 
    };
    
    m_ui->on_stop_all_sounds = [this]() { 
        handle_stop_all_sounds(); 
    };
}

void ImGuiPlayer::process_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
#ifdef IMGUI_USE_SDL3
        ImGui_ImplSDL3_ProcessEvent(&event);
        
        if (event.type == SDL_EVENT_QUIT) {
            m_running = false;
        }
        
        // SDL3 uses different event types
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            m_running = false;
        }
#else
        ImGui_ImplSDL2_ProcessEvent(&event);
        
        if (event.type == SDL_QUIT) {
            m_running = false;
        }
        
        if (event.type == SDL_WINDOWEVENT && 
            event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(m_window)) {
            m_running = false;
        }
#endif
    }
}

void ImGuiPlayer::update() {
    // Clean up finished sound streams
    if (m_stream_manager) {
        m_stream_manager->cleanup_finished_streams();
    }
}

void ImGuiPlayer::render() {
    // Start the Dear ImGui frame
#ifdef IMGUI_USE_SDL3
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
#else
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
#endif
    ImGui::NewFrame();
    
    // Render UI
    if (m_ui) {
        m_ui->render();
    }
    
    // Rendering
    ImGui::Render();
    
    // Clear and render
    SDL_SetRenderDrawColor(m_renderer, 115, 140, 153, 255);
    SDL_RenderClear(m_renderer);
#ifdef IMGUI_USE_SDL3
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
#else
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
#endif
    SDL_RenderPresent(m_renderer);
}

void ImGuiPlayer::cleanup_imgui() {
#ifdef IMGUI_USE_SDL3
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
#else
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
#endif
    ImGui::DestroyContext();
}

void ImGuiPlayer::cleanup_sdl() {
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    SDL_Quit();
}

void ImGuiPlayer::handle_refresh_devices() {
    if (m_device_manager) {
        m_device_manager->refresh_device_list();
    }
}

void ImGuiPlayer::handle_switch_device(int device_index) {
    if (!m_device_manager) {
        return;
    }
    
    // Stop all streams before switching
    if (m_stream_manager) {
        m_stream_manager->stop_all_streams();
    }
    
    // Switch device in a separate thread to avoid blocking UI
    std::thread([this, device_index]() {
        m_device_manager->switch_device(device_index);
    }).detach();
}

void ImGuiPlayer::handle_play_music(test_data::music_type type) {
    if (m_stream_manager) {
        m_stream_manager->play_music(type);
    }
}

void ImGuiPlayer::handle_stop_music() {
    if (m_stream_manager) {
        m_stream_manager->stop_music();
    }
}

void ImGuiPlayer::handle_play_sound(test_data::music_type type) {
    if (m_stream_manager) {
        m_stream_manager->play_sound(type);
    }
}

void ImGuiPlayer::handle_stop_all_sounds() {
    if (m_stream_manager) {
        m_stream_manager->stop_all_sounds();
    }
}

} // namespace musac::player