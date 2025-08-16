# Volume and Panning Guide

This guide covers all aspects of audio volume control and stereo positioning in Musac, including per-stream and global controls, mute functionality, and advanced mixing techniques.

## Table of Contents

1. [Volume Control Basics](#volume-control-basics)
2. [Stereo Positioning (Panning)](#stereo-positioning-panning)
3. [Mute Functionality](#mute-functionality)
4. [Per-Stream vs Global Control](#per-stream-vs-global-control)
5. [Advanced Volume Techniques](#advanced-volume-techniques)
6. [3D Audio Positioning](#3d-audio-positioning)
7. [Best Practices](#best-practices)
8. [Troubleshooting](#troubleshooting)

## Volume Control Basics

### Setting Volume

Volume in Musac uses a linear scale where:
- `0.0` = silence
- `1.0` = normal/default volume
- `>1.0` = amplification (use with caution)

```cpp
#include <musac/audio_device.hh>
#include <musac/audio_loader.hh>

musac::audio_device device;
device.open_default_device();

auto source = musac::audio_loader::load("music.mp3");
auto stream = device.create_stream(std::move(source));

// Set volume before playing
stream->set_volume(0.5f);  // 50% volume
stream->play();

// Change volume while playing
stream->set_volume(0.8f);  // 80% volume

// Get current volume
float current = stream->get_volume();
std::cout << "Current volume: " << (current * 100) << "%\n";
```

### Volume Range and Amplification

```cpp
// Safe volume range
stream->set_volume(0.0f);   // Silent
stream->set_volume(0.5f);   // Half volume
stream->set_volume(1.0f);   // Normal volume

// Amplification (use carefully to avoid clipping)
stream->set_volume(1.5f);   // 150% - may cause distortion
stream->set_volume(2.0f);   // 200% - likely to clip

// Clamp volume to safe range
float safe_volume(float vol) {
    return std::clamp(vol, 0.0f, 1.0f);
}
```

### Smooth Volume Changes

```cpp
class VolumeController {
private:
    musac::audio_stream* m_stream;
    std::thread m_fade_thread;
    std::atomic<bool> m_fading{false};
    
public:
    void smooth_volume_change(musac::audio_stream* stream,
                             float target_volume,
                             std::chrono::milliseconds duration) {
        m_stream = stream;
        
        if (m_fading.load()) {
            // Cancel previous fade
            m_fading.store(false);
            if (m_fade_thread.joinable()) {
                m_fade_thread.join();
            }
        }
        
        m_fading.store(true);
        m_fade_thread = std::thread([this, target_volume, duration]() {
            float start_volume = m_stream->get_volume();
            float delta = target_volume - start_volume;
            
            const int steps = 50;  // Number of interpolation steps
            auto step_duration = duration / steps;
            
            for (int i = 1; i <= steps && m_fading.load(); ++i) {
                float progress = static_cast<float>(i) / steps;
                
                // Use ease-in-out curve for smoother transition
                progress = smooth_step(progress);
                
                float volume = start_volume + delta * progress;
                m_stream->set_volume(volume);
                
                std::this_thread::sleep_for(step_duration);
            }
            
            m_fading.store(false);
        });
    }
    
private:
    float smooth_step(float t) {
        // Smooth cubic curve
        return t * t * (3.0f - 2.0f * t);
    }
};
```

## Stereo Positioning (Panning)

### Basic Panning

Panning values range from -1.0 (full left) to 1.0 (full right):

```cpp
// Set stereo position
stream->set_stereo_pos(-1.0f);  // Full left
stream->set_stereo_pos(0.0f);   // Center (default)
stream->set_stereo_pos(1.0f);   // Full right
stream->set_stereo_pos(-0.5f);  // Slightly left
stream->set_stereo_pos(0.3f);   // Slightly right

// Get current position
float pos = stream->get_stereo_pos();
```

### Panning Laws

Different panning laws for various use cases:

```cpp
class PanningController {
public:
    enum PanLaw {
        LINEAR,        // Simple linear panning
        CONSTANT_POWER, // -3dB center, maintains perceived loudness
        EQUAL_POWER    // -6dB center, true equal power
    };
    
private:
    PanLaw m_law = CONSTANT_POWER;
    
public:
    void apply_panning(musac::audio_stream* stream, float pan) {
        // Clamp pan to valid range
        pan = std::clamp(pan, -1.0f, 1.0f);
        
        float left_gain, right_gain;
        
        switch (m_law) {
            case LINEAR:
                left_gain = (pan <= 0) ? 1.0f : (1.0f - pan);
                right_gain = (pan >= 0) ? 1.0f : (1.0f + pan);
                break;
                
            case CONSTANT_POWER:
                // -3dB at center
                {
                    float angle = (pan + 1.0f) * 0.25f * M_PI;
                    left_gain = std::cos(angle);
                    right_gain = std::sin(angle);
                }
                break;
                
            case EQUAL_POWER:
                // -6dB at center  
                {
                    float angle = (pan + 1.0f) * 0.25f * M_PI;
                    left_gain = std::sqrt(std::cos(angle));
                    right_gain = std::sqrt(std::sin(angle));
                }
                break;
        }
        
        stream->set_stereo_pos(pan);
        // Note: Musac handles the actual channel gains internally
    }
    
    void set_pan_law(PanLaw law) {
        m_law = law;
    }
};
```

### Auto-Panning Effects

```cpp
class AutoPanner {
private:
    musac::audio_stream* m_stream;
    std::thread m_pan_thread;
    std::atomic<bool> m_running{false};
    
public:
    enum WaveShape {
        SINE,
        TRIANGLE,
        SQUARE,
        SAWTOOTH
    };
    
    void start_auto_pan(musac::audio_stream* stream,
                        float rate_hz = 1.0f,
                        float depth = 1.0f,
                        WaveShape shape = SINE) {
        m_stream = stream;
        m_running.store(true);
        
        m_pan_thread = std::thread([this, rate_hz, depth, shape]() {
            auto period = std::chrono::milliseconds(
                static_cast<int>(1000.0f / rate_hz)
            );
            
            float phase = 0.0f;
            const float phase_increment = 0.01f;  // 100 updates per period
            auto update_interval = period / 100;
            
            while (m_running.load()) {
                float pan = calculate_pan(phase, depth, shape);
                m_stream->set_stereo_pos(pan);
                
                phase += phase_increment;
                if (phase >= 1.0f) phase -= 1.0f;
                
                std::this_thread::sleep_for(update_interval);
            }
        });
    }
    
    void stop_auto_pan() {
        m_running.store(false);
        if (m_pan_thread.joinable()) {
            m_pan_thread.join();
        }
        m_stream->set_stereo_pos(0.0f);  // Reset to center
    }
    
private:
    float calculate_pan(float phase, float depth, WaveShape shape) {
        float value;
        
        switch (shape) {
            case SINE:
                value = std::sin(phase * 2.0f * M_PI);
                break;
                
            case TRIANGLE:
                value = (phase < 0.5f) 
                    ? (4.0f * phase - 1.0f)
                    : (3.0f - 4.0f * phase);
                break;
                
            case SQUARE:
                value = (phase < 0.5f) ? -1.0f : 1.0f;
                break;
                
            case SAWTOOTH:
                value = 2.0f * phase - 1.0f;
                break;
        }
        
        return value * depth;
    }
};
```

## Mute Functionality

### Stream Muting

```cpp
class MuteController {
private:
    struct MuteState {
        musac::audio_stream* stream;
        float previous_volume;
        bool is_muted;
    };
    
    std::unordered_map<musac::audio_stream*, MuteState> m_states;
    
public:
    void mute(musac::audio_stream* stream) {
        auto& state = m_states[stream];
        
        if (!state.is_muted) {
            state.stream = stream;
            state.previous_volume = stream->get_volume();
            state.is_muted = true;
            stream->set_volume(0.0f);
        }
    }
    
    void unmute(musac::audio_stream* stream) {
        auto it = m_states.find(stream);
        if (it != m_states.end() && it->second.is_muted) {
            stream->set_volume(it->second.previous_volume);
            it->second.is_muted = false;
        }
    }
    
    void toggle_mute(musac::audio_stream* stream) {
        auto it = m_states.find(stream);
        if (it != m_states.end() && it->second.is_muted) {
            unmute(stream);
        } else {
            mute(stream);
        }
    }
    
    bool is_muted(musac::audio_stream* stream) const {
        auto it = m_states.find(stream);
        return it != m_states.end() && it->second.is_muted;
    }
};
```

### Soft Mute with Fade

```cpp
class SoftMuteController {
private:
    std::unordered_map<musac::audio_stream*, float> m_saved_volumes;
    
public:
    void soft_mute(musac::audio_stream* stream,
                   std::chrono::milliseconds fade_time = 
                       std::chrono::milliseconds(50)) {
        // Save current volume
        m_saved_volumes[stream] = stream->get_volume();
        
        // Fade to silence
        fade_to_volume(stream, 0.0f, fade_time);
    }
    
    void soft_unmute(musac::audio_stream* stream,
                     std::chrono::milliseconds fade_time = 
                         std::chrono::milliseconds(100)) {
        auto it = m_saved_volumes.find(stream);
        if (it != m_saved_volumes.end()) {
            // Fade back to previous volume
            fade_to_volume(stream, it->second, fade_time);
            m_saved_volumes.erase(it);
        }
    }
    
private:
    void fade_to_volume(musac::audio_stream* stream,
                       float target,
                       std::chrono::milliseconds duration) {
        // Implementation similar to smooth_volume_change above
        // ...
    }
};
```

## Per-Stream vs Global Control

### Per-Stream Volume Management

```cpp
class StreamVolumeManager {
private:
    struct StreamSettings {
        float base_volume = 1.0f;
        float category_multiplier = 1.0f;
        float master_multiplier = 1.0f;
        bool muted = false;
    };
    
    std::unordered_map<musac::audio_stream*, StreamSettings> m_streams;
    
public:
    void register_stream(musac::audio_stream* stream,
                        float base_volume = 1.0f) {
        m_streams[stream].base_volume = base_volume;
        update_stream_volume(stream);
    }
    
    void set_stream_volume(musac::audio_stream* stream, float volume) {
        if (auto it = m_streams.find(stream); it != m_streams.end()) {
            it->second.base_volume = volume;
            update_stream_volume(stream);
        }
    }
    
    void set_category_volume(const std::string& category, float volume) {
        // Update all streams in category
        for (auto& [stream, settings] : m_streams) {
            if (get_stream_category(stream) == category) {
                settings.category_multiplier = volume;
                update_stream_volume(stream);
            }
        }
    }
    
private:
    void update_stream_volume(musac::audio_stream* stream) {
        auto& settings = m_streams[stream];
        
        if (settings.muted) {
            stream->set_volume(0.0f);
        } else {
            float final_volume = settings.base_volume * 
                               settings.category_multiplier * 
                               settings.master_multiplier;
            stream->set_volume(final_volume);
        }
    }
    
    std::string get_stream_category(musac::audio_stream* stream) {
        // Return category based on stream metadata or type
        return "default";
    }
};
```

### Global Volume Control

```cpp
class GlobalVolumeController {
private:
    float m_master_volume = 1.0f;
    float m_music_volume = 1.0f;
    float m_sfx_volume = 1.0f;
    float m_voice_volume = 1.0f;
    
    struct CategoryVolumes {
        std::vector<musac::audio_stream*> music_streams;
        std::vector<musac::audio_stream*> sfx_streams;
        std::vector<musac::audio_stream*> voice_streams;
    } m_categories;
    
public:
    enum Category {
        MUSIC,
        SFX,
        VOICE
    };
    
    void register_stream(musac::audio_stream* stream, Category category) {
        switch (category) {
            case MUSIC:
                m_categories.music_streams.push_back(stream);
                break;
            case SFX:
                m_categories.sfx_streams.push_back(stream);
                break;
            case VOICE:
                m_categories.voice_streams.push_back(stream);
                break;
        }
        update_stream(stream, category);
    }
    
    void set_master_volume(float volume) {
        m_master_volume = std::clamp(volume, 0.0f, 1.0f);
        update_all_streams();
    }
    
    void set_category_volume(Category category, float volume) {
        volume = std::clamp(volume, 0.0f, 1.0f);
        
        switch (category) {
            case MUSIC:
                m_music_volume = volume;
                update_category(m_categories.music_streams, MUSIC);
                break;
            case SFX:
                m_sfx_volume = volume;
                update_category(m_categories.sfx_streams, SFX);
                break;
            case VOICE:
                m_voice_volume = volume;
                update_category(m_categories.voice_streams, VOICE);
                break;
        }
    }
    
private:
    void update_stream(musac::audio_stream* stream, Category category) {
        float category_vol = 1.0f;
        
        switch (category) {
            case MUSIC: category_vol = m_music_volume; break;
            case SFX: category_vol = m_sfx_volume; break;
            case VOICE: category_vol = m_voice_volume; break;
        }
        
        float final_volume = m_master_volume * category_vol;
        stream->set_volume(final_volume);
    }
    
    void update_category(const std::vector<musac::audio_stream*>& streams,
                        Category category) {
        for (auto stream : streams) {
            update_stream(stream, category);
        }
    }
    
    void update_all_streams() {
        update_category(m_categories.music_streams, MUSIC);
        update_category(m_categories.sfx_streams, SFX);
        update_category(m_categories.voice_streams, VOICE);
    }
};
```

## Advanced Volume Techniques

### Dynamic Range Compression

```cpp
class DynamicRangeCompressor {
private:
    float m_threshold = 0.8f;    // Start compression above this level
    float m_ratio = 4.0f;         // Compression ratio (4:1)
    float m_attack_ms = 10.0f;    // Attack time
    float m_release_ms = 100.0f;  // Release time
    float m_makeup_gain = 1.0f;   // Compensate for volume reduction
    
public:
    float process_volume(float input_volume) {
        if (input_volume <= m_threshold) {
            return input_volume * m_makeup_gain;
        }
        
        // Apply compression above threshold
        float excess = input_volume - m_threshold;
        float compressed_excess = excess / m_ratio;
        float output = m_threshold + compressed_excess;
        
        return output * m_makeup_gain;
    }
    
    void apply_to_stream(musac::audio_stream* stream) {
        float current = stream->get_volume();
        float compressed = process_volume(current);
        stream->set_volume(compressed);
    }
};
```

### Ducking System

```cpp
class AudioDuckingSystem {
private:
    struct DuckingGroup {
        std::vector<musac::audio_stream*> streams;
        float normal_volume = 1.0f;
        float ducked_volume = 0.3f;
        bool is_ducked = false;
    };
    
    std::unordered_map<std::string, DuckingGroup> m_groups;
    
public:
    void create_ducking_group(const std::string& name,
                             float normal_vol = 1.0f,
                             float ducked_vol = 0.3f) {
        m_groups[name] = DuckingGroup{
            {}, normal_vol, ducked_vol, false
        };
    }
    
    void add_to_group(const std::string& group_name,
                      musac::audio_stream* stream) {
        if (auto it = m_groups.find(group_name); it != m_groups.end()) {
            it->second.streams.push_back(stream);
            stream->set_volume(it->second.normal_volume);
        }
    }
    
    void duck_group(const std::string& group_name,
                    std::chrono::milliseconds fade_time = 
                        std::chrono::milliseconds(100)) {
        if (auto it = m_groups.find(group_name); it != m_groups.end()) {
            auto& group = it->second;
            if (!group.is_ducked) {
                group.is_ducked = true;
                for (auto stream : group.streams) {
                    fade_to(stream, group.ducked_volume, fade_time);
                }
            }
        }
    }
    
    void unduck_group(const std::string& group_name,
                     std::chrono::milliseconds fade_time = 
                         std::chrono::milliseconds(200)) {
        if (auto it = m_groups.find(group_name); it != m_groups.end()) {
            auto& group = it->second;
            if (group.is_ducked) {
                group.is_ducked = false;
                for (auto stream : group.streams) {
                    fade_to(stream, group.normal_volume, fade_time);
                }
            }
        }
    }
    
    void play_priority_sound(musac::audio_stream* priority_stream,
                            const std::string& duck_group) {
        // Duck background audio
        duck_group(duck_group);
        
        // Set callback to unduck when priority sound finishes
        priority_stream->set_finished_callback(
            [this, duck_group](musac::audio_stream*) {
                this->unduck_group(duck_group);
            }
        );
        
        priority_stream->play();
    }
    
private:
    void fade_to(musac::audio_stream* stream,
                float target,
                std::chrono::milliseconds duration) {
        // Smooth fade implementation
        // ...
    }
};
```

## 3D Audio Positioning

### Simple 3D Positioning

```cpp
class Simple3DPositioner {
public:
    struct Position3D {
        float x, y, z;
    };
    
    struct AudioSettings {
        float volume;
        float pan;
    };
    
    AudioSettings calculate_3d_audio(const Position3D& source,
                                    const Position3D& listener,
                                    float max_distance = 100.0f) {
        // Calculate distance
        float dx = source.x - listener.x;
        float dy = source.y - listener.y;
        float dz = source.z - listener.z;
        float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        // Calculate volume based on distance
        float volume = 1.0f - (distance / max_distance);
        volume = std::clamp(volume, 0.0f, 1.0f);
        
        // Apply inverse square law for more realistic falloff
        volume = volume * volume;
        
        // Calculate pan based on horizontal position
        float pan = std::atan2(dx, dz) / M_PI;  // -1 to 1
        pan = std::clamp(pan, -1.0f, 1.0f);
        
        // Reduce panning effect for distant sounds
        pan *= volume;
        
        return {volume, pan};
    }
    
    void apply_3d_position(musac::audio_stream* stream,
                          const Position3D& source,
                          const Position3D& listener) {
        auto settings = calculate_3d_audio(source, listener);
        stream->set_volume(settings.volume);
        stream->set_stereo_pos(settings.pan);
    }
};
```

### HRTF Simulation (Simple)

```cpp
class SimpleHRTF {
private:
    // Simplified head-related transfer function
    float calculate_ear_delay(float angle_degrees) {
        // Maximum ITD (Interaural Time Difference) ~0.7ms
        const float max_delay_ms = 0.7f;
        return max_delay_ms * std::sin(angle_degrees * M_PI / 180.0f);
    }
    
    float calculate_ear_gain(float angle_degrees, bool left_ear) {
        // Simple head shadow effect
        float normalized_angle = angle_degrees / 90.0f;
        
        if (left_ear) {
            // Left ear: full gain at -90°, reduced at +90°
            return 1.0f - (normalized_angle * 0.3f);
        } else {
            // Right ear: reduced at -90°, full gain at +90°
            return 1.0f + (normalized_angle * 0.3f);
        }
    }
    
public:
    void apply_hrtf(musac::audio_stream* stream,
                   float azimuth_degrees,
                   float elevation_degrees) {
        // Simplified HRTF - just handles azimuth
        float pan = azimuth_degrees / 90.0f;
        pan = std::clamp(pan, -1.0f, 1.0f);
        
        // Apply elevation effect (higher = quieter)
        float elevation_factor = 1.0f - (std::abs(elevation_degrees) / 90.0f) * 0.3f;
        
        stream->set_stereo_pos(pan);
        stream->set_volume(stream->get_volume() * elevation_factor);
    }
};
```

## Best Practices

### 1. Volume Normalization

```cpp
class VolumeNormalizer {
public:
    static float normalize_volume(float raw_volume,
                                 float min_db = -60.0f,
                                 float max_db = 0.0f) {
        // Convert to dB
        float db = 20.0f * std::log10(std::max(raw_volume, 0.001f));
        
        // Clamp to range
        db = std::clamp(db, min_db, max_db);
        
        // Normalize to 0-1
        return (db - min_db) / (max_db - min_db);
    }
    
    static float db_to_linear(float db) {
        return std::pow(10.0f, db / 20.0f);
    }
    
    static float linear_to_db(float linear) {
        return 20.0f * std::log10(std::max(linear, 0.001f));
    }
};
```

### 2. Smooth Transitions

```cpp
// Always use smooth transitions for user-facing volume changes
void user_volume_change(musac::audio_stream* stream,
                       float new_volume) {
    const auto fade_time = std::chrono::milliseconds(50);
    smooth_volume_change(stream, new_volume, fade_time);
}

// Instant changes only for system events
void system_volume_change(musac::audio_stream* stream,
                         float new_volume) {
    stream->set_volume(new_volume);
}
```

### 3. Prevent Clipping

```cpp
class ClippingPrevention {
public:
    static float prevent_clipping(float volume,
                                 float headroom_db = -3.0f) {
        float max_linear = VolumeNormalizer::db_to_linear(headroom_db);
        return std::min(volume, max_linear);
    }
    
    static void apply_limiter(musac::audio_stream* stream,
                             float threshold = 0.95f) {
        float current = stream->get_volume();
        if (current > threshold) {
            // Soft limiting
            float excess = current - threshold;
            float limited = threshold + (excess * 0.1f);  // 10:1 ratio
            stream->set_volume(limited);
        }
    }
};
```

### 4. Save/Restore Settings

```cpp
class VolumeSettings {
public:
    struct Settings {
        float master_volume = 1.0f;
        float music_volume = 1.0f;
        float sfx_volume = 1.0f;
        float voice_volume = 1.0f;
        bool muted = false;
    };
    
    void save_to_file(const Settings& settings,
                     const std::string& filename) {
        // Save to JSON or binary file
        std::ofstream file(filename);
        file << settings.master_volume << "\n";
        file << settings.music_volume << "\n";
        file << settings.sfx_volume << "\n";
        file << settings.voice_volume << "\n";
        file << settings.muted << "\n";
    }
    
    Settings load_from_file(const std::string& filename) {
        Settings settings;
        std::ifstream file(filename);
        if (file) {
            file >> settings.master_volume;
            file >> settings.music_volume;
            file >> settings.sfx_volume;
            file >> settings.voice_volume;
            file >> settings.muted;
        }
        return settings;
    }
};
```

## Troubleshooting

### Volume Not Working

```cpp
void diagnose_volume_issue(musac::audio_stream* stream) {
    std::cout << "Volume Diagnostics:\n";
    std::cout << "  Stream volume: " << stream->get_volume() << "\n";
    std::cout << "  Is playing: " << stream->is_playing() << "\n";
    std::cout << "  Is paused: " << stream->is_paused() << "\n";
    
    // Test volume changes
    float original = stream->get_volume();
    stream->set_volume(1.0f);
    std::cout << "  After set to 1.0: " << stream->get_volume() << "\n";
    stream->set_volume(original);
    
    // Check for muting
    if (stream->get_volume() == 0.0f) {
        std::cout << "  WARNING: Volume is 0 (muted)\n";
    }
}
```

### Panning Issues

```cpp
void test_panning(musac::audio_stream* stream) {
    std::cout << "Testing panning...\n";
    
    // Test left
    stream->set_stereo_pos(-1.0f);
    std::cout << "Left speaker only (3 seconds)\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Test center
    stream->set_stereo_pos(0.0f);
    std::cout << "Center (both speakers) (3 seconds)\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Test right
    stream->set_stereo_pos(1.0f);
    std::cout << "Right speaker only (3 seconds)\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Reset
    stream->set_stereo_pos(0.0f);
}
```

## See Also

- [Playing Audio Files](PLAYING_AUDIO_FILES.md)
- [Playback Control Guide](PLAYBACK_CONTROL.md)
- [Device Management Guide](DEVICE_MANAGEMENT.md)
- [API Reference](doxygen/html/index.html)