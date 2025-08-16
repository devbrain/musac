# Getting Started with Musac

## Introduction

Musac is a modular C++ audio library designed for games and multimedia applications. It provides:

- Simple and intuitive API for audio playback
- Support for many audio formats (WAV, MP3, FLAC, OGG Vorbis, MOD, and more)
- Cross-platform audio device management
- Built-in mixer with per-stream volume control
- PC speaker emulation for retro games
- Thread-safe operation

## Installation

### Prerequisites

- C++17 compatible compiler (GCC 8+, Clang 9+, MSVC 2019+)
- CMake 3.15 or higher
- Ninja build system (recommended) or Make
- SDL2 or SDL3 (for audio device access)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/musac.git
cd musac

# Configure the build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build the library
cmake --build build

# Run tests (optional)
cd build && ctest
```

### Installation

```bash
# Install to system (may require sudo)
cmake --install build

# Or install to custom prefix
cmake --install build --prefix ~/my-libs
```

## Quick Start

### Playing Your First Sound

Here's the simplest way to play an audio file:

```cpp
#include <musac/audio_device.hh>
#include <musac/audio_loader.hh>
#include <iostream>

int main() {
    try {
        // Open the default audio device
        musac::audio_device device;
        device.open_default_device();
        
        // Load and play an audio file
        auto source = musac::audio_loader::load("music.mp3");
        auto stream = device.create_stream(std::move(source));
        stream->play();
        
        // Wait for playback to finish
        std::cout << "Playing music... Press Enter to stop.\n";
        std::cin.get();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}
```

### Compiling Your Program

With pkg-config:
```bash
g++ -std=c++17 main.cpp $(pkg-config --cflags --libs musac)
```

With CMake:
```cmake
find_package(musac REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app musac::musac)
```

## Basic Concepts

### Audio Device

The `audio_device` class manages the connection to your audio hardware:

```cpp
musac::audio_device device;

// Option 1: Use default device
device.open_default_device();

// Option 2: Choose specific device
auto devices = device.enumerate_devices();
for (const auto& info : devices) {
    std::cout << info.id << ": " << info.name << '\n';
}
device.open_device(devices[0].id);
```

### Audio Sources

Audio sources represent the data to be played. They can be created from:

- Files on disk
- Memory buffers
- Custom generators

```cpp
// From file
auto source1 = musac::audio_loader::load("sound.wav");

// From memory
std::vector<uint8_t> data = load_file_data();
auto source2 = musac::audio_loader::load_from_memory(data);

// With automatic format detection
auto stream = musac::io_from_file("music.ogg", "rb");
musac::audio_source source3(std::move(stream));
```

### Audio Streams

Streams control playback of audio sources:

```cpp
auto stream = device.create_stream(std::move(source));

// Playback control
stream->play();          // Start playback
stream->pause();         // Pause (can resume)
stream->resume();        // Resume from pause
stream->stop();          // Stop (with fade-out)

// Volume control
stream->set_volume(0.5f);      // 50% volume
stream->set_stereo_pos(-0.5f); // Pan left

// Status
bool playing = stream->is_playing();
bool paused = stream->is_paused();
```

## Common Use Cases

### Playing Background Music with Looping

```cpp
auto music = musac::audio_loader::load("background.mp3");
auto stream = device.create_stream(std::move(music));

// Loop forever
stream->play(musac::infinite_loop);

// Or loop 3 times
stream->play(3);
```

### Sound Effects Pool

```cpp
// Preload sound effects
std::vector<std::unique_ptr<musac::audio_stream>> fx_pool;

for (int i = 0; i < 10; ++i) {
    auto source = musac::audio_loader::load("explosion.wav");
    fx_pool.push_back(device.create_stream(std::move(source)));
}

// Play a sound effect
for (auto& fx : fx_pool) {
    if (!fx->is_playing()) {
        fx->rewind();
        fx->play();
        break;
    }
}
```

### Fade In/Out

```cpp
// Fade in over 2 seconds
stream->play(1, std::chrono::seconds(2));

// Fade out over 1 second when stopping
stream->stop(std::chrono::seconds(1));
```

### PC Speaker for Retro Games

```cpp
auto pc_speaker = device.create_pc_speaker_stream();
pc_speaker->play();

// Simple beep
pc_speaker->beep();

// Play a melody using MML (Music Macro Language)
pc_speaker->play_mml("T120 L4 C D E F G A B >C");

// Custom tones
using namespace std::chrono_literals;
pc_speaker->sound(440.0f, 500ms);  // A4 for half second
pc_speaker->sound(880.0f, 500ms);  // A5
```

## Supported Formats

Musac automatically detects and plays these formats:

| Format | Extension | Description |
|--------|-----------|-------------|
| WAV | .wav | Uncompressed PCM audio |
| MP3 | .mp3 | MPEG Layer 3 compressed audio |
| FLAC | .flac | Lossless compressed audio |
| Vorbis | .ogg | OGG Vorbis compressed audio |
| AIFF | .aiff, .aif | Apple/SGI audio format |
| VOC | .voc | Creative Labs audio format |
| MOD | .mod, .xm, .it, .s3m | Tracker music formats |
| VGM | .vgm, .vgz | Video game music |
| CMF | .cmf | Creative Music File |

## Error Handling

Musac uses exceptions for error reporting:

```cpp
try {
    device.open_default_device();
} catch (const musac::device_error& e) {
    // Handle device-specific errors
    std::cerr << "Device error: " << e.what() << '\n';
} catch (const musac::format_error& e) {
    // Handle format/codec errors
    std::cerr << "Format error: " << e.what() << '\n';
} catch (const musac::decoder_error& e) {
    // Handle decoder errors
    std::cerr << "Decoder error: " << e.what() << '\n';
}
```

## Thread Safety

- Audio devices and streams are thread-safe for control operations
- The audio callback runs on a separate thread managed by the backend
- Multiple streams can play simultaneously through the built-in mixer
- Stream destruction is synchronized with the audio callback

## Performance Tips

1. **Choose appropriate formats**:
   - Use WAV for short sound effects (no decoding overhead)
   - Use FLAC for high-quality music (lossless compression)
   - Use OGG Vorbis for background music (good compression)

2. **Preload sound effects**:
   - Load frequently used sounds into memory
   - Create stream pools for concurrent playback

3. **Buffer sizing**:
   - Larger buffers reduce CPU usage but increase latency
   - Default settings work well for most applications

## Next Steps

- Read the [API Reference](doxygen/html/index.html) for detailed documentation
- Check out the [Examples](../examples/) directory
- Learn about [Custom Backends](BACKEND_DEVELOPMENT.md)
- Explore [Advanced Features](ADVANCED_USAGE.md)

## Getting Help

- [GitHub Issues](https://github.com/yourusername/musac/issues)
- [API Documentation](doxygen/html/index.html)
- [Discord Community](https://discord.gg/yourinvite)