# Musac - Modular Audio Library

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-064F8C?logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

A modern, modular C++ audio library providing comprehensive audio playback, format support, and device management with automatic sample rate conversion.

## Features

- **Backend-Agnostic Architecture**: Core library independent of audio system
- **Multi-Backend Support**: SDL3, SDL2, or custom audio backends
- **Extensive Format Support**: 
  - Lossless: WAV, FLAC, AIFF
  - Lossy: MP3, Vorbis/OGG
  - Retro: MOD, CMF, VGM, VOC, 8SVX, MML
  - Compressed: IMA ADPCM, µ-law, A-law
  - Sequence: MIDI, MUS, XMI, HMI/HMP
- **Automatic Sample Rate Conversion**: Transparent resampling with high-quality Speex resampler
- **Advanced Audio Control**: Volume, stereo positioning, fading, looping
- **Modular Architecture**: Clean separation between SDK, codecs, and backends
- **Real-Time Safe**: Lock-free audio callbacks with thread-safe mixing
- **Format Auto-Detection**: Automatic codec selection based on file content

## Installation

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16 or later
- Optional: SDL3 or SDL2 for audio backend

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/musac.git
cd musac

# Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run tests (optional)
cd build && ctest
```

### Build Options

#### Library Types

**Shared Libraries** (`.so`, `.dll`, `.dylib`) - Default
- Smaller executable size
- Runtime linking allows updates without recompilation
- Memory shared between processes
- Requires library to be present at runtime

**Static Libraries** (`.a`, `.lib`)
- Larger executable size (library code embedded)
- No runtime dependencies
- Better optimization opportunities
- Preferred for distribution without dependencies

**Position-Independent Code (PIC)**
- Required when static libraries will be linked into shared libraries
- Allows code to be loaded at any memory address
- Small performance overhead but necessary for certain use cases
- Enable with `MUSAC_BUILD_FPIC=ON` when building static libraries that will be used in shared libraries

#### CMake Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `MUSAC_BUILD_SHARED` | ON | Build as shared library (.so/.dll/.dylib) |
| `MUSAC_BUILD_FPIC` | OFF | Build static library with position-independent code |
| `MUSAC_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer and UndefinedBehaviorSanitizer |
| `MUSAC_ENABLE_LTO` | OFF | Enable link-time optimization |
| `MUSAC_BUILD_SDL3_BACKEND` | ON | Build SDL3 audio backend |
| `MUSAC_BUILD_SDL2_BACKEND` | ON | Build SDL2 audio backend |
| `MUSAC_BUILD_TESTS` | ON | Build unit tests |
| `MUSAC_BUILD_EXAMPLES` | ON | Build example programs |
| `MUSAC_BUILD_BINDINGS` | OFF | Build language bindings |
| `MUSAC_BUILD_JAVA_BINDING` | ON | Build Java/JNI binding (if bindings enabled) |
| `MUSAC_BUILD_PYTHON_BINDING` | OFF | Build Python binding (if bindings enabled) |

#### Example Configurations

```bash
# Build with SDL2 backend only
cmake -B build -DMUSAC_BUILD_SDL3_BACKEND=OFF -DMUSAC_BUILD_SDL2_BACKEND=ON

# Static library build with position-independent code
cmake -B build -DMUSAC_BUILD_SHARED=OFF -DMUSAC_BUILD_FPIC=ON

# Debug build with sanitizers
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DMUSAC_ENABLE_SANITIZERS=ON

# Release build with optimizations
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMUSAC_ENABLE_LTO=ON

# Minimal build (no tests, examples, or bindings)
cmake -B build -DMUSAC_BUILD_TESTS=OFF -DMUSAC_BUILD_EXAMPLES=OFF -DMUSAC_BUILD_BINDINGS=OFF

# Build with Java bindings
cmake -B build -DMUSAC_BUILD_BINDINGS=ON -DMUSAC_BUILD_JAVA_BINDING=ON
```

## Quick Start

### Basic Playback

```cpp
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/audio_source.hh>
#include <musac/stream.hh>

int main() {
    // Initialize audio system
    auto backend = musac::create_sdl3_backend();
    auto registry = musac::create_registry_with_all_codecs();
    musac::audio_system::init(backend, registry);
    
    // Open default audio device
    auto device = musac::audio_device::open_default_device(backend);
    
    // Load and play audio file
    auto io = musac::io_from_file("music.mp3", "rb");
    musac::audio_source source(std::move(io));  // Auto-detects format
    auto stream = device.create_stream(std::move(source));
    
    stream.play();
    
    // Wait for playback to finish
    while (stream.is_playing()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    musac::audio_system::done();
    return 0;
}
```

### Volume Control

```cpp
stream.set_volume(0.5f);        // 50% volume
stream.set_stereo_pos(-0.5f);   // Pan left
stream.fade_in(1000);           // Fade in over 1 second
```

### Format Detection

```cpp
// The library automatically detects the format
auto io = musac::io_from_file("unknown_format.audio", "rb");
musac::audio_source source(std::move(io));  // Works with any supported format
```

## Architecture

### Backend-Agnostic Design

Musac is designed to be **backend-agnostic**, meaning the core library doesn't depend on any specific audio system. This provides:

- **Portability** - Easy to port to new platforms
- **Flexibility** - Choose the best backend for your platform
- **Extensibility** - Easy to add custom backends for specialized hardware
- **Testability** - Backend interface allows for test implementations

### Audio Backends

A backend is a pluggable module that handles the platform-specific audio output. Musac includes several backends:

| Backend | Platform | Use Case | Notes |
|---------|----------|----------|--------|
| **SDL3** | Cross-platform | Modern applications | Recommended for new projects |
| **SDL2** | Cross-platform | Legacy compatibility | Stable, widely deployed |
| **Custom** | Any | Specialized hardware | Implement your own backend |

The backend interface is simple and well-defined, making it easy to add support for platform-specific audio APIs like WASAPI, ALSA, CoreAudio, or custom hardware.

#### Using Different Backends

```cpp
// Use SDL3 backend (default)
auto backend = musac::create_sdl3_backend();

// Use SDL2 backend  
auto backend = musac::create_sdl2_backend();

// Create custom backend (implement backend interface)
auto backend = std::make_shared<MyCustomBackend>();

// Initialize audio system with chosen backend
musac::audio_system::init(backend, registry);
```

### Library Structure

```
musac/
├── musac_sdk/          # Core SDK (format-agnostic, backend-agnostic)
│   ├── Types & interfaces
│   ├── Audio conversion
│   └── I/O abstractions
├── musac_codecs/       # Format decoders (backend-agnostic)
│   ├── WAV, MP3, FLAC
│   ├── AIFF, VOC, 8SVX
│   └── MOD, VGM, etc.
├── musac/              # Main library (backend-agnostic core)
│   ├── Audio device management
│   ├── Mixing & streaming
│   └── Backend interface
└── backends/           # Backend implementations (platform-specific)
    ├── SDL3 backend
    └── SDL2 backend
```

## Key Features in Detail

### Automatic Sample Rate Conversion

The library automatically handles sample rate mismatches between files and devices:

```cpp
// 8kHz file plays correctly on 48kHz device
auto source = musac::audio_source(io_8khz);  // Automatic resampling
```

### Thread-Safe Architecture

- Lock-free audio callbacks for real-time performance
- Thread-safe mixer with concurrent stream operations
- Weak pointer lifetime management prevents use-after-free

### Comprehensive Format Support

| Format | Type | Features | Library | License |
|--------|------|----------|---------|---------|
| WAV | Lossless | PCM, float, ADPCM | dr_wav | Public Domain |
| FLAC | Lossless | Full spec support | dr_flac | Public Domain |
| MP3 | Lossy | MPEG-1/2/2.5 Layer III | dr_mp3 | Public Domain |
| Vorbis/OGG | Lossy | OGG container | stb_vorbis | Public Domain |
| AIFF | Lossless | PCM, float, IMA4, µ-law | Native | MIT |
| VOC | Retro | Creative Labs format | Native | MIT |
| 8SVX | Retro | Amiga IFF audio | Native | MIT |
| MOD | Tracker | ProTracker, FastTracker | libmodplug | Public Domain |
| VGM | Chiptune | Video game music | Native | MIT |
| CMF | MIDI-like | Creative Music File | Native | MIT |
| MML | Text | Music Macro Language | Native | MIT |
| MIDI | Sequence | Standard MIDI (Type 0/1) | Native + OPL | MIT |
| MUS | Sequence | DOOM music format | Native + OPL | MIT |
| XMI | Sequence | Extended MIDI (Origin games) | Native + OPL | MIT |
| HMI/HMP | Sequence | Human Machine Interfaces | Native + OPL | MIT |

### Error Handling

Exception-based error handling with detailed context:

```cpp
try {
    decoder.open(io.get());
} catch (const musac::decoder_error& e) {
    std::cerr << "Failed to open: " << e.what() << std::endl;
}
```

## Documentation

- API Reference (coming soon)
- Architecture Guide (coming soon)
- Format Support Details (coming soon)
- [Examples](example/)

## Testing

The project includes comprehensive testing:

```bash
# Run all tests
cd build && ctest

# Run specific test suite
./bin/musac_unittest -tc="*AIFF*"

# Run with verbose output
./bin/musac_unittest -s
```

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **dr_libs** - Audio format decoding (MP3, FLAC, WAV)
- **stb_vorbis** - Vorbis/OGG decoding
- **libmodplug** - MOD/tracker format support
- **Speex** - High-quality audio resampling
- **SDL** - Cross-platform audio backend

## Status

- Core audio playback - Complete
- Format detection & decoding - Complete
- Automatic resampling - Complete
- Thread-safe mixing - Complete
- Volume & effects - Complete
- Spatial audio - In Progress


## Contact

For questions, bug reports, or feature requests, please [open an issue](https://github.com/yourusername/musac/issues).