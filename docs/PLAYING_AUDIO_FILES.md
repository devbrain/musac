# Playing Audio Files

This guide covers everything you need to know about loading and playing audio files with the Musac library.

## Table of Contents

1. [Loading Audio Files](#loading-audio-files)
2. [Format Auto-Detection](#format-auto-detection)
3. [Error Handling](#error-handling)
4. [Complete Examples](#complete-examples)
5. [Performance Considerations](#performance-considerations)
6. [Troubleshooting](#troubleshooting)

## Loading Audio Files

Musac provides several ways to load audio files depending on your needs.

### Method 1: Using audio_loader (Simplest)

The `audio_loader` class provides the easiest way to load audio files:

```cpp
#include <musac/audio_loader.hh>
#include <musac/audio_device.hh>

// Load from file path
auto source = musac::audio_loader::load("sounds/music.mp3");

// Create stream and play
musac::audio_device device;
device.open_default_device();
auto stream = device.create_stream(std::move(source));
stream->play();
```

### Method 2: Using io_stream (More Control)

For more control over file loading, use the io_stream interface:

```cpp
#include <musac/sdk/io_stream.hh>
#include <musac/audio_source.hh>

// Create file stream
auto io = musac::io_from_file("music.ogg", "rb");
if (!io) {
    throw std::runtime_error("Failed to open file");
}

// Create audio source
musac::audio_source source(std::move(io));
source.open(44100, 2, 1024);  // sample rate, channels, buffer size
```

### Method 3: Loading from Memory

Load audio data from memory buffers:

```cpp
// Load file into memory
std::vector<uint8_t> file_data = read_file_to_memory("sound.wav");

// Create memory stream
auto io = musac::io_from_memory(file_data);

// Create audio source
musac::audio_source source(std::move(io));
source.open(44100, 2, 1024);
```

### Method 4: Using Decoders Directly

For maximum control, use decoders directly:

```cpp
#include <musac/codecs/decoder_drmp3.hh>

auto io = musac::io_from_file("music.mp3", "rb");

// Create and open decoder
musac::decoder_drmp3 decoder;
decoder.open(io.get());

// Get audio properties
auto channels = decoder.get_channels();
auto sample_rate = decoder.get_rate();
auto duration = decoder.duration();

// Create source from decoder
auto source = musac::audio_source::from_decoder(
    std::make_unique<musac::decoder_drmp3>(std::move(decoder))
);
```

## Format Auto-Detection

Musac automatically detects audio formats using the `decoders_registry`.

### How Auto-Detection Works

1. **File Extension**: First attempts detection by file extension
2. **Magic Numbers**: Checks file headers for format signatures
3. **Content Analysis**: Some formats require deeper inspection

```cpp
#include <musac/codecs/decoders_registry.hh>

// Check if a file can be decoded
auto io = musac::io_from_file("unknown.audio", "rb");
auto decoder = musac::decoders_registry::instance().create_decoder(io.get());

if (decoder) {
    std::cout << "Format detected: " << decoder->get_name() << "\n";
} else {
    std::cout << "Unknown format\n";
}
```

### Supported Formats

| Format | Extensions | Detection Method | Priority |
|--------|------------|------------------|----------|
| WAV | .wav | "RIFF" header | 10 |
| MP3 | .mp3 | Frame sync pattern | 20 |
| FLAC | .flac | "fLaC" header | 15 |
| OGG Vorbis | .ogg, .oga | "OggS" header | 18 |
| AIFF | .aiff, .aif | "FORM" + "AIFF" | 22 |
| 8SVX | .8svx, .iff | "FORM" + "8SVX" | 23 |
| VOC | .voc | "Creative Voice File" | 25 |
| MOD/XM/IT/S3M | .mod, .xm, .it, .s3m | Pattern signatures | 30 |
| VGM | .vgm, .vgz | "Vgm " header | 35 |
| MIDI/MUS/XMI | .mid, .mus, .xmi | Format headers | 40 |
| CMF | .cmf | "CTMF" header | 45 |
| MML | .mml | Text pattern analysis | 50 |

### Forcing a Specific Decoder

```cpp
#include <musac/codecs/decoder_vorbis.hh>

// Force Vorbis decoder
auto io = musac::io_from_file("audio.dat", "rb");
musac::decoder_vorbis decoder;

if (musac::decoder_vorbis::accept(io.get())) {
    decoder.open(io.get());
    // Use decoder...
} else {
    throw std::runtime_error("Not a Vorbis file");
}
```

## Error Handling

Musac uses exceptions for error reporting. Always wrap file operations in try-catch blocks.

### Common Exceptions

```cpp
try {
    auto source = musac::audio_loader::load("music.mp3");
    auto stream = device.create_stream(std::move(source));
    stream->play();
    
} catch (const musac::io_error& e) {
    // File not found, permission denied, etc.
    std::cerr << "IO Error: " << e.what() << "\n";
    
} catch (const musac::format_error& e) {
    // Unsupported or corrupted format
    std::cerr << "Format Error: " << e.what() << "\n";
    
} catch (const musac::decoder_error& e) {
    // Decoder-specific errors
    std::cerr << "Decoder Error: " << e.what() << "\n";
    
} catch (const musac::device_error& e) {
    // Audio device errors
    std::cerr << "Device Error: " << e.what() << "\n";
    
} catch (const std::exception& e) {
    // Other errors
    std::cerr << "Error: " << e.what() << "\n";
}
```

### Validation Before Playing

```cpp
bool validate_audio_file(const std::string& path) {
    try {
        auto io = musac::io_from_file(path, "rb");
        if (!io) {
            std::cerr << "Cannot open file: " << path << "\n";
            return false;
        }
        
        auto decoder = musac::decoders_registry::instance()
            .create_decoder(io.get());
        if (!decoder) {
            std::cerr << "Unsupported format: " << path << "\n";
            return false;
        }
        
        decoder->open(io.get());
        
        // Check properties
        auto channels = decoder->get_channels();
        auto rate = decoder->get_rate();
        
        if (channels == 0 || channels > 8) {
            std::cerr << "Invalid channel count: " << channels << "\n";
            return false;
        }
        
        if (rate < 8000 || rate > 192000) {
            std::cerr << "Unusual sample rate: " << rate << "\n";
            // This is a warning, not necessarily an error
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Validation failed: " << e.what() << "\n";
        return false;
    }
}
```

## Complete Examples

### Example 1: Simple Music Player

```cpp
#include <musac/audio_device.hh>
#include <musac/audio_loader.hh>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <audio_file>\n";
        return 1;
    }
    
    try {
        // Initialize audio system
        musac::audio_device device;
        device.open_default_device();
        
        // Load audio file
        std::cout << "Loading: " << argv[1] << "\n";
        auto source = musac::audio_loader::load(argv[1]);
        
        // Create stream
        auto stream = device.create_stream(std::move(source));
        
        // Get duration
        auto duration = stream->duration();
        if (duration.count() > 0) {
            auto seconds = duration.count() / 1000000.0;
            std::cout << "Duration: " << seconds << " seconds\n";
        }
        
        // Play with fade-in
        std::cout << "Playing...\n";
        stream->play(1, std::chrono::seconds(2));
        
        // Wait for playback to finish
        while (stream->is_playing()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Print progress
            auto pos = stream->tell_time();
            auto secs = pos.count() / 1000000.0;
            std::cout << "\rPosition: " << secs << " seconds" << std::flush;
        }
        
        std::cout << "\nPlayback complete.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
```

### Example 2: Sound Effect Manager

```cpp
#include <musac/audio_device.hh>
#include <musac/audio_loader.hh>
#include <unordered_map>
#include <vector>
#include <memory>

class SoundEffectManager {
private:
    musac::audio_device m_device;
    
    struct SoundEffect {
        std::string path;
        std::vector<std::unique_ptr<musac::audio_stream>> pool;
        size_t next_index = 0;
    };
    
    std::unordered_map<std::string, SoundEffect> m_effects;
    
public:
    SoundEffectManager() {
        m_device.open_default_device();
    }
    
    void load_effect(const std::string& name, 
                     const std::string& path, 
                     size_t pool_size = 3) {
        SoundEffect& effect = m_effects[name];
        effect.path = path;
        
        // Create pool of streams for concurrent playback
        for (size_t i = 0; i < pool_size; ++i) {
            auto source = musac::audio_loader::load(path);
            effect.pool.push_back(
                m_device.create_stream(std::move(source))
            );
        }
    }
    
    void play_effect(const std::string& name, 
                     float volume = 1.0f,
                     float pan = 0.0f) {
        auto it = m_effects.find(name);
        if (it == m_effects.end()) {
            throw std::runtime_error("Unknown effect: " + name);
        }
        
        SoundEffect& effect = it->second;
        
        // Find available stream in pool
        for (auto& stream : effect.pool) {
            if (!stream->is_playing()) {
                stream->rewind();
                stream->set_volume(volume);
                stream->set_stereo_pos(pan);
                stream->play();
                return;
            }
        }
        
        // All streams busy, use round-robin
        auto& stream = effect.pool[effect.next_index];
        effect.next_index = (effect.next_index + 1) % effect.pool.size();
        
        stream->stop();  // Stop current playback
        stream->rewind();
        stream->set_volume(volume);
        stream->set_stereo_pos(pan);
        stream->play();
    }
};

// Usage
int main() {
    try {
        SoundEffectManager sfx;
        
        // Load effects
        sfx.load_effect("explosion", "sounds/explosion.wav", 5);
        sfx.load_effect("coin", "sounds/coin.wav", 3);
        sfx.load_effect("jump", "sounds/jump.wav", 2);
        
        // Play effects
        sfx.play_effect("jump");
        sfx.play_effect("coin", 0.8f, -0.5f);  // 80% volume, pan left
        sfx.play_effect("explosion", 1.0f, 0.3f);  // Full volume, pan right
        
        // Wait for effects to play
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
```

### Example 3: Playlist Player

```cpp
#include <musac/audio_device.hh>
#include <musac/audio_loader.hh>
#include <vector>
#include <string>

class PlaylistPlayer {
private:
    musac::audio_device m_device;
    std::vector<std::string> m_playlist;
    size_t m_current_track = 0;
    std::unique_ptr<musac::audio_stream> m_stream;
    bool m_loop_playlist = false;
    
public:
    PlaylistPlayer() {
        m_device.open_default_device();
    }
    
    void add_track(const std::string& path) {
        m_playlist.push_back(path);
    }
    
    void play() {
        if (m_playlist.empty()) return;
        
        play_track(m_current_track);
    }
    
    void next() {
        if (m_playlist.empty()) return;
        
        m_current_track = (m_current_track + 1) % m_playlist.size();
        play_track(m_current_track);
    }
    
    void previous() {
        if (m_playlist.empty()) return;
        
        if (m_current_track == 0) {
            m_current_track = m_playlist.size() - 1;
        } else {
            m_current_track--;
        }
        play_track(m_current_track);
    }
    
    void update() {
        if (m_stream && !m_stream->is_playing()) {
            // Track finished, play next
            if (m_current_track + 1 < m_playlist.size()) {
                next();
            } else if (m_loop_playlist) {
                m_current_track = 0;
                play_track(0);
            }
        }
    }
    
    void set_loop(bool loop) {
        m_loop_playlist = loop;
    }
    
private:
    void play_track(size_t index) {
        if (index >= m_playlist.size()) return;
        
        // Stop current track
        if (m_stream) {
            m_stream->stop(std::chrono::milliseconds(500));
        }
        
        // Load and play new track
        auto source = musac::audio_loader::load(m_playlist[index]);
        m_stream = m_device.create_stream(std::move(source));
        m_stream->play(1, std::chrono::seconds(1));  // Fade in
        
        std::cout << "Now playing: " << m_playlist[index] << "\n";
    }
};
```

## Performance Considerations

### Preloading vs Streaming

**Preloading** (default):
- Entire file loaded into memory
- Best for short sounds (<10MB)
- No disk I/O during playback
- Lower latency

**Streaming** (for large files):
- Data loaded in chunks
- Best for music and long audio
- Requires continuous disk I/O
- Higher memory efficiency

### Format Selection

| Use Case | Recommended Format | Reason |
|----------|-------------------|---------|
| Sound Effects | WAV | No decoding overhead, instant playback |
| Background Music | OGG Vorbis | Good compression, low CPU usage |
| High Quality Music | FLAC | Lossless, reasonable file size |
| Voice/Dialog | MP3 or OGG | Good compression for speech |
| Retro Games | MOD/XM | Tiny file size, authentic sound |

### Buffer Size Tuning

```cpp
// Smaller buffer = lower latency, higher CPU
source.open(44100, 2, 512);   // 11ms latency

// Default buffer = balanced
source.open(44100, 2, 1024);  // 23ms latency

// Larger buffer = higher latency, lower CPU
source.open(44100, 2, 4096);  // 92ms latency
```

### Concurrent Playback

```cpp
// Limit concurrent streams for performance
const size_t MAX_CONCURRENT_SOUNDS = 32;
size_t active_streams = 0;

// Before playing a new sound
if (active_streams < MAX_CONCURRENT_SOUNDS) {
    stream->play();
    active_streams++;
}

// Monitor and clean up finished streams
for (auto& stream : all_streams) {
    if (!stream->is_playing()) {
        active_streams--;
    }
}
```

## Troubleshooting

### Common Issues and Solutions

**File Not Found**
```cpp
// Check file exists before loading
#include <filesystem>

if (!std::filesystem::exists(path)) {
    std::cerr << "File not found: " << path << "\n";
    return;
}
```

**Unsupported Format**
```cpp
// List supported decoders
auto& registry = musac::decoders_registry::instance();
std::cout << "Supported formats:\n";
for (const auto& decoder_name : registry.get_decoder_names()) {
    std::cout << "  - " << decoder_name << "\n";
}
```

**Corrupted File**
```cpp
// Validate file before playing
try {
    auto io = musac::io_from_file(path, "rb");
    auto decoder = registry.create_decoder(io.get());
    decoder->open(io.get());
    
    // Try to decode a small portion
    float test_buffer[1024];
    bool more_data;
    decoder->decode(test_buffer, 1024, more_data);
    
    std::cout << "File appears valid\n";
} catch (const std::exception& e) {
    std::cout << "File may be corrupted: " << e.what() << "\n";
}
```

**No Audio Output**
```cpp
// Check device is open
if (!device.is_open()) {
    std::cerr << "Audio device not open\n";
}

// Check stream state
std::cout << "Playing: " << stream->is_playing() << "\n";
std::cout << "Paused: " << stream->is_paused() << "\n";
std::cout << "Volume: " << stream->get_volume() << "\n";

// Check system volume
std::cout << "Ensure system volume is not muted\n";
```

**Performance Issues**
```cpp
// Monitor decoder performance
auto start = std::chrono::steady_clock::now();
size_t samples = decoder->decode(buffer, buffer_size, more);
auto end = std::chrono::steady_clock::now();

auto duration = std::chrono::duration_cast<std::chrono::microseconds>
    (end - start).count();
    
std::cout << "Decode time: " << duration << "μs for " 
          << samples << " samples\n";
```

## See Also

- [Playback Control Guide](PLAYBACK_CONTROL.md)
- [Volume and Panning Guide](VOLUME_AND_PANNING.md)
- [Looping Audio Guide](LOOPING_AUDIO.md)
- [Error Handling Guide](ERROR_HANDLING_GUIDE.md)
- [API Reference](doxygen/html/index.html)