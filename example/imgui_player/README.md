# ImGui Player - Refactored Version

This is a refactored version of the original imgui_player example, demonstrating improved code organization and maintainability.

## Architecture Improvements

### 1. **Separation of Concerns**
The monolithic `ImGuiPlayer` class has been split into focused components:

- **AudioDeviceManager**: Handles audio device enumeration and switching
- **StreamManager**: Manages audio stream lifecycle and playback
- **WaveformVisualizer**: Handles audio visualization
- **PlayerUI**: Manages all UI rendering
- **ImGuiPlayer**: Main application coordinator

### 2. **Single Responsibility Principle**
Each class now has one clear responsibility:

```cpp
// Before: One class doing everything
class ImGuiPlayer {
    void render() { /* 240+ lines handling everything */ }
    // Device management, streams, UI, visualization all mixed
};

// After: Focused classes
class AudioDeviceManager { /* Only device management */ };
class StreamManager { /* Only stream lifecycle */ };
class PlayerUI { /* Only UI rendering */ };
```

### 3. **Improved Maintainability**

#### Before:
- Single 592-line file
- Mixed concerns (UI + audio + device management)
- Large methods (render() was 240+ lines)
- Scattered SDL2/SDL3 conditional compilation

#### After:
- Modular design with separate files
- Each file ~150-200 lines
- Focused methods (<50 lines each)
- Centralized compatibility handling

### 4. **Better Error Handling**
- Each component handles its own errors
- Clear error propagation
- Graceful degradation

### 5. **Dependency Injection**
Components are loosely coupled through dependency injection:

```cpp
// UI doesn't directly create managers
m_ui->set_device_manager(m_device_manager.get());
m_ui->set_stream_manager(m_stream_manager.get());

// UI communicates through callbacks
m_ui->on_play_music = [this](auto type) { 
    handle_play_music(type); 
};
```

## File Structure

```
imgui_player/
├── README.md                    # This file
├── CMakeLists.txt              # Build configuration
├── main.cc                     # Entry point
├── imgui_player.hh/cc          # Main application coordinator
├── audio_device_manager.hh/cc  # Device management
├── stream_manager.hh/cc        # Stream lifecycle
├── waveform_visualizer.hh/cc   # Audio visualization
└── player_ui.hh/cc             # UI rendering
```

## Class Responsibilities

### AudioDeviceManager
- Enumerate audio devices
- Switch between devices
- Provide device information
- Handle device lifecycle

### StreamManager
- Create audio streams from files
- Manage music playback (single stream)
- Manage sound effects (multiple streams)
- Handle volume control
- Clean up finished streams

### WaveformVisualizer
- Fetch audio samples
- Process samples for visualization
- Render waveform using ImGui
- Provide visualization settings

### PlayerUI
- Render all UI components
- Handle user interactions
- Manage UI state
- Delegate actions to managers via callbacks

### ImGuiPlayer (Main)
- Initialize SDL/OpenGL/ImGui
- Create and connect components
- Run main event loop
- Handle cleanup

## Benefits of Refactoring

1. **Easier Testing**: Each component can be tested independently
2. **Better Reusability**: Components can be reused in other projects
3. **Clearer Code**: Each file has a clear, focused purpose
4. **Easier Maintenance**: Changes are localized to specific components
5. **Better Collaboration**: Multiple developers can work on different components
6. **Improved Debugging**: Issues are easier to locate and fix
7. **Future-Proof**: Easy to add new features without affecting existing code

## Building

```bash
# From the musac root directory
cmake -B build
cmake --build build --target imgui_player_refactored
```

## Running

```bash
./build/bin/imgui_player_refactored
```

## Extending

To add new features:

1. **New Audio Effect**: Modify `StreamManager`
2. **New Visualization**: Extend `WaveformVisualizer`
3. **New UI Section**: Add to `PlayerUI`
4. **New Device Feature**: Update `AudioDeviceManager`

Each change is localized to its relevant component.