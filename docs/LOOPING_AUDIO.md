# Looping Audio Guide

This guide covers all aspects of looping audio in Musac, including loop count control, infinite loops, loop callbacks, and gapless looping techniques.

## Table of Contents

1. [Basic Looping](#basic-looping)
2. [Loop Count Control](#loop-count-control)
3. [Infinite Loops](#infinite-loops)
4. [Loop Callbacks](#loop-callbacks)
5. [Gapless Looping](#gapless-looping)
6. [Advanced Looping Patterns](#advanced-looping-patterns)
7. [Format-Specific Looping](#format-specific-looping)
8. [Best Practices](#best-practices)

## Basic Looping

### Simple Loop Playback

The simplest way to loop audio is to specify a loop count when calling `play()`:

```cpp
#include <musac/audio_device.hh>
#include <musac/audio_loader.hh>

musac::audio_device device;
device.open_default_device();

auto source = musac::audio_loader::load("background_music.ogg");
auto stream = device.create_stream(std::move(source));

// Play the audio 3 times
stream->play(3);

// Play once (no loop)
stream->play(1);  // or just stream->play()
```

### Loop Status

Check the current loop status:

```cpp
// Check if stream is set to loop
int loop_count = stream->get_loop_count();
if (loop_count == musac::infinite_loop) {
    std::cout << "Looping forever\n";
} else if (loop_count > 1) {
    std::cout << "Looping " << loop_count << " times\n";
} else {
    std::cout << "Not looping\n";
}

// Get remaining loops
int remaining = stream->get_loops_remaining();
std::cout << "Loops remaining: " << remaining << "\n";
```

## Loop Count Control

### Setting Loop Count

```cpp
// Set loop count before playing
stream->set_loop_count(5);
stream->play();

// Or set it during play
stream->play(5);

// Change loop count while playing
stream->set_loop_count(10);  // Extends to 10 total loops
```

### Dynamic Loop Control

```cpp
class DynamicLooper {
private:
    musac::audio_stream* m_stream;
    int m_base_loops = 3;
    
public:
    DynamicLooper(musac::audio_stream* stream) : m_stream(stream) {}
    
    void play_with_intro(int intro_loops, int main_loops) {
        // Play intro section
        m_stream->play(intro_loops);
        
        // Set up callback to switch to main loop
        m_stream->set_loop_callback(
            [this, intro_loops, main_loops](musac::audio_stream* s, int remaining) {
                if (remaining == 0) {
                    // Intro finished, start main loop
                    s->set_loop_count(main_loops);
                    s->play(main_loops);
                }
            }
        );
    }
    
    void extend_loop(int additional_loops) {
        int current = m_stream->get_loops_remaining();
        m_stream->set_loop_count(current + additional_loops);
    }
    
    void fade_out_after_loop() {
        // Finish current loop then fade out
        m_stream->set_loop_count(1);
        m_stream->set_loop_callback(
            [](musac::audio_stream* s, int remaining) {
                if (remaining == 0) {
                    s->stop(std::chrono::seconds(2));  // 2-second fade
                }
            }
        );
    }
};
```

## Infinite Loops

### Creating Infinite Loops

```cpp
// Method 1: Using the constant
stream->play(musac::infinite_loop);

// Method 2: Using -1 (same as infinite_loop)
stream->play(-1);

// Method 3: Set before playing
stream->set_loop_count(musac::infinite_loop);
stream->play();
```

### Managing Infinite Loops

```cpp
class InfiniteLoopManager {
private:
    musac::audio_stream* m_stream;
    std::atomic<bool> m_should_stop{false};
    std::thread m_monitor_thread;
    
public:
    void start_infinite_loop(musac::audio_stream* stream) {
        m_stream = stream;
        m_stream->play(musac::infinite_loop);
        
        // Start monitoring thread
        m_monitor_thread = std::thread([this]() {
            while (!m_should_stop.load()) {
                // Check conditions for stopping
                if (should_stop_loop()) {
                    stop_after_current_loop();
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }
    
    void stop_after_current_loop() {
        // Change from infinite to 1 (finish current loop)
        m_stream->set_loop_count(1);
    }
    
    void stop_immediately() {
        m_should_stop.store(true);
        m_stream->stop(std::chrono::seconds(1));  // Fade out
        if (m_monitor_thread.joinable()) {
            m_monitor_thread.join();
        }
    }
    
private:
    bool should_stop_loop() {
        // Custom logic to determine when to stop
        // e.g., level complete, time limit reached, etc.
        return false;
    }
};
```

## Loop Callbacks

### Setting Up Loop Callbacks

Loop callbacks are triggered at the end of each loop iteration:

```cpp
stream->set_loop_callback(
    [](musac::audio_stream* stream, int loops_remaining) {
        std::cout << "Loop completed. ";
        if (loops_remaining == musac::infinite_loop) {
            std::cout << "Continuing infinite loop\n";
        } else if (loops_remaining > 0) {
            std::cout << loops_remaining << " loops remaining\n";
        } else {
            std::cout << "Final loop completed\n";
        }
    }
);
```

### Advanced Callback Usage

```cpp
class LoopEventHandler {
private:
    int m_loop_number = 0;
    std::function<void(int)> m_on_loop;
    std::function<void()> m_on_complete;
    
public:
    void setup(musac::audio_stream* stream) {
        stream->set_loop_callback(
            [this](musac::audio_stream* s, int remaining) {
                m_loop_number++;
                
                if (m_on_loop) {
                    m_on_loop(m_loop_number);
                }
                
                // Trigger events based on loop number
                handle_loop_event(m_loop_number);
                
                if (remaining == 0 && m_on_complete) {
                    m_on_complete();
                }
            }
        );
    }
    
    void on_each_loop(std::function<void(int)> callback) {
        m_on_loop = callback;
    }
    
    void on_loops_complete(std::function<void()> callback) {
        m_on_complete = callback;
    }
    
private:
    void handle_loop_event(int loop_num) {
        switch (loop_num) {
            case 3:
                std::cout << "Halfway through loops\n";
                break;
            case 5:
                std::cout << "Almost done\n";
                break;
        }
    }
};
```

## Gapless Looping

### Ensuring Gapless Playback

Musac provides gapless looping by default, but here are techniques to ensure perfect loops:

```cpp
class GaplessLooper {
private:
    musac::audio_device m_device;
    
public:
    std::unique_ptr<musac::audio_stream> 
    create_gapless_loop(const std::string& file) {
        auto source = musac::audio_loader::load(file);
        auto stream = m_device.create_stream(std::move(source));
        
        // Ensure buffer is large enough for smooth looping
        stream->set_buffer_size(4096);
        
        // Pre-buffer before starting
        stream->prebuffer();
        
        return stream;
    }
    
    void play_seamless_loop(musac::audio_stream* stream) {
        // Disable any fade effects that might cause gaps
        stream->set_fade_in_duration(std::chrono::milliseconds(0));
        
        // Start infinite loop
        stream->play(musac::infinite_loop);
    }
};
```

### Loop Point Detection

For formats with embedded loop points:

```cpp
class LoopPointHandler {
public:
    struct LoopInfo {
        std::chrono::microseconds loop_start;
        std::chrono::microseconds loop_end;
        bool has_loop_points;
    };
    
    LoopInfo detect_loop_points(musac::decoder* decoder) {
        LoopInfo info;
        
        // Some formats (like VGM, MOD) have built-in loop points
        info.loop_start = decoder->get_loop_start();
        info.loop_end = decoder->get_loop_end();
        info.has_loop_points = (info.loop_start.count() >= 0);
        
        return info;
    }
    
    void setup_loop_region(musac::audio_stream* stream, 
                          const LoopInfo& info) {
        if (info.has_loop_points) {
            stream->set_loop_region(info.loop_start, info.loop_end);
        }
        // Otherwise loops entire track
    }
};
```

### Creating Perfect Loops

Tips for creating audio files that loop perfectly:

```cpp
class PerfectLoopCreator {
public:
    bool verify_loop_compatibility(const std::string& file) {
        auto source = musac::audio_loader::load(file);
        auto decoder = source.get_decoder();
        
        // Check if duration is exact
        auto duration = decoder->duration();
        auto samples = decoder->get_total_samples();
        auto rate = decoder->get_rate();
        
        // Calculate expected samples
        auto expected_samples = (duration.count() * rate) / 1000000;
        
        // Check for partial samples that might cause gaps
        if (samples != expected_samples) {
            std::cout << "Warning: File may have partial samples\n";
            return false;
        }
        
        return true;
    }
    
    void prepare_for_loop(musac::audio_stream* stream) {
        // Ensure no silence at start/end
        stream->trim_silence();
        
        // Disable any automatic fades
        stream->set_auto_fade(false);
        
        // Use exact sample boundaries
        stream->set_sample_accurate_looping(true);
    }
};
```

## Advanced Looping Patterns

### A-B Loop (Loop Section)

```cpp
class ABLooper {
private:
    musac::audio_stream* m_stream;
    std::chrono::microseconds m_point_a;
    std::chrono::microseconds m_point_b;
    bool m_ab_active = false;
    
public:
    void set_loop_points(std::chrono::microseconds a, 
                        std::chrono::microseconds b) {
        m_point_a = a;
        m_point_b = b;
    }
    
    void enable_ab_loop(musac::audio_stream* stream) {
        m_stream = stream;
        m_ab_active = true;
        
        // Set position callback to monitor playback
        stream->set_position_callback(
            [this](musac::audio_stream* s, std::chrono::microseconds pos) {
                if (m_ab_active && pos >= m_point_b) {
                    // Jump back to point A
                    s->seek_to_time(m_point_a);
                }
            },
            std::chrono::milliseconds(10)  // Check every 10ms
        );
        
        // Start playing from point A
        stream->seek_to_time(m_point_a);
        stream->play();
    }
    
    void disable_ab_loop() {
        m_ab_active = false;
        // Continue playing normally
    }
};
```

### Crossfade Loop

```cpp
class CrossfadeLooper {
private:
    musac::audio_device m_device;
    std::unique_ptr<musac::audio_stream> m_stream1;
    std::unique_ptr<musac::audio_stream> m_stream2;
    musac::audio_stream* m_active;
    musac::audio_stream* m_inactive;
    
public:
    void setup(const std::string& file) {
        // Create two streams with the same audio
        auto source1 = musac::audio_loader::load(file);
        auto source2 = musac::audio_loader::load(file);
        
        m_stream1 = m_device.create_stream(std::move(source1));
        m_stream2 = m_device.create_stream(std::move(source2));
        
        m_active = m_stream1.get();
        m_inactive = m_stream2.get();
    }
    
    void start_crossfade_loop(std::chrono::milliseconds fade_duration) {
        m_active->play();
        
        // Set up callback for crossfade
        m_active->set_position_callback(
            [this, fade_duration](musac::audio_stream* s, 
                                 std::chrono::microseconds pos) {
                auto duration = s->duration();
                auto remaining = duration - pos;
                
                // Start crossfade near the end
                if (remaining <= fade_duration) {
                    start_crossfade(fade_duration);
                }
            },
            std::chrono::milliseconds(100)
        );
    }
    
private:
    void start_crossfade(std::chrono::milliseconds duration) {
        // Start the inactive stream
        m_inactive->rewind();
        m_inactive->set_volume(0.0f);
        m_inactive->play();
        
        // Crossfade
        float active_start = m_active->get_volume();
        auto start_time = std::chrono::steady_clock::now();
        
        while (true) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                          (now - start_time);
            
            if (elapsed >= duration) break;
            
            float progress = static_cast<float>(elapsed.count()) / 
                           duration.count();
            
            m_active->set_volume(active_start * (1.0f - progress));
            m_inactive->set_volume(active_start * progress);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Swap active/inactive
        m_active->stop();
        std::swap(m_active, m_inactive);
    }
};
```

### Layered Loops

```cpp
class LayeredLoopSystem {
private:
    struct Layer {
        std::unique_ptr<musac::audio_stream> stream;
        float target_volume;
        bool active;
    };
    
    std::vector<Layer> m_layers;
    musac::audio_device m_device;
    
public:
    void add_layer(const std::string& file, float volume = 1.0f) {
        auto source = musac::audio_loader::load(file);
        
        Layer layer;
        layer.stream = m_device.create_stream(std::move(source));
        layer.target_volume = volume;
        layer.active = false;
        
        m_layers.push_back(std::move(layer));
    }
    
    void start_all_loops() {
        // Start all layers in sync
        for (auto& layer : m_layers) {
            layer.stream->set_volume(layer.active ? layer.target_volume : 0.0f);
            layer.stream->play(musac::infinite_loop);
        }
    }
    
    void enable_layer(size_t index, std::chrono::milliseconds fade_time) {
        if (index >= m_layers.size()) return;
        
        auto& layer = m_layers[index];
        if (layer.active) return;
        
        layer.active = true;
        fade_volume(layer.stream.get(), layer.target_volume, fade_time);
    }
    
    void disable_layer(size_t index, std::chrono::milliseconds fade_time) {
        if (index >= m_layers.size()) return;
        
        auto& layer = m_layers[index];
        if (!layer.active) return;
        
        layer.active = false;
        fade_volume(layer.stream.get(), 0.0f, fade_time);
    }
    
private:
    void fade_volume(musac::audio_stream* stream, float target, 
                     std::chrono::milliseconds duration) {
        // Smooth volume fade implementation
        // ... (similar to previous examples)
    }
};
```

## Format-Specific Looping

### MOD/Tracker Music

MOD files often have built-in loop patterns:

```cpp
// MOD files typically loop by default
auto mod_source = musac::audio_loader::load("music.mod");
auto mod_stream = device.create_stream(std::move(mod_source));

// Many MOD files loop infinitely by design
mod_stream->play();  // Will loop according to pattern data
```

### VGM (Video Game Music)

VGM files have explicit loop points:

```cpp
auto vgm_source = musac::audio_loader::load("level.vgm");
auto vgm_stream = device.create_stream(std::move(vgm_source));

// VGM files with loop data will loop automatically
// Check if file has loop information
if (vgm_stream->has_loop_points()) {
    std::cout << "VGM has built-in loop points\n";
    vgm_stream->play(musac::infinite_loop);
}
```

### WAV with Metadata

Some WAV files contain loop metadata:

```cpp
class WAVLoopReader {
public:
    struct WAVLoopInfo {
        uint32_t loop_start_sample;
        uint32_t loop_end_sample;
        bool has_loops;
    };
    
    WAVLoopInfo read_loop_info(const std::string& file) {
        // Read WAV chunks looking for 'smpl' chunk
        // which contains loop information
        WAVLoopInfo info;
        // ... implementation
        return info;
    }
};
```

## Best Practices

### 1. Choose Appropriate Loop Methods

```cpp
// Short sound effects - finite loops
sfx_stream->play(3);

// Background music - infinite loops
music_stream->play(musac::infinite_loop);

// Dynamic music - use callbacks
dynamic_stream->set_loop_callback(handle_loop_transition);
```

### 2. Prepare Audio for Looping

```cpp
// Remove silence from start/end
void prepare_loop_file(musac::audio_stream* stream) {
    // Ensure clean loop points
    stream->trim_silence();
    
    // Disable automatic fades
    stream->set_fade_in_duration(std::chrono::milliseconds(0));
    
    // Pre-buffer for smooth start
    stream->prebuffer();
}
```

### 3. Handle Loop Transitions

```cpp
class LoopTransitionManager {
public:
    void smooth_loop_transition(musac::audio_stream* stream) {
        stream->set_loop_callback(
            [](musac::audio_stream* s, int remaining) {
                // Brief fade at loop point for smoother transition
                if (remaining > 0) {
                    // Quick fade out/in
                    s->set_volume(0.95f);
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(10)
                    );
                    s->set_volume(1.0f);
                }
            }
        );
    }
};
```

### 4. Memory Management for Loops

```cpp
// For long-running loops, monitor memory
class LoopMemoryManager {
    void monitor_loop_memory(musac::audio_stream* stream) {
        stream->set_loop_callback(
            [](musac::audio_stream* s, int remaining) {
                static int loop_count = 0;
                loop_count++;
                
                // Periodically clean up resources
                if (loop_count % 100 == 0) {
                    // Force garbage collection if needed
                    s->compact_buffers();
                }
            }
        );
    }
};
```

### 5. Testing Loop Quality

```cpp
void test_loop_quality(const std::string& file) {
    auto source = musac::audio_loader::load(file);
    auto stream = device.create_stream(std::move(source));
    
    // Test short loops to hear transitions clearly
    stream->play(5);
    
    // Listen for:
    // - Clicks or pops at loop points
    // - Timing irregularities
    // - Volume changes
    
    stream->set_loop_callback(
        [](musac::audio_stream* s, int remaining) {
            std::cout << "Loop point reached - check for artifacts\n";
        }
    );
}
```

## Troubleshooting

### Gaps in Loops

```cpp
// Diagnose gap issues
void diagnose_loop_gaps(musac::audio_stream* stream) {
    auto duration = stream->duration();
    auto samples = stream->get_total_samples();
    auto rate = stream->get_rate();
    
    std::cout << "Duration: " << duration.count() << " μs\n";
    std::cout << "Samples: " << samples << "\n";
    std::cout << "Rate: " << rate << " Hz\n";
    
    // Check for partial frames
    auto frames = samples / stream->get_channels();
    if (frames * stream->get_channels() != samples) {
        std::cout << "Warning: Partial frame detected\n";
    }
}
```

### Loop Not Working

```cpp
void debug_loop(musac::audio_stream* stream) {
    std::cout << "Loop count: " << stream->get_loop_count() << "\n";
    std::cout << "Is playing: " << stream->is_playing() << "\n";
    std::cout << "Position: " << stream->tell_time().count() << "\n";
    
    // Ensure stream hasn't finished
    if (!stream->is_playing()) {
        std::cout << "Stream has stopped - check for errors\n";
    }
}
```

## See Also

- [Playing Audio Files](PLAYING_AUDIO_FILES.md)
- [Playback Control Guide](PLAYBACK_CONTROL.md)
- [Audio Source Guide](AUDIO_SOURCE_GUIDE.md)
- [API Reference](doxygen/html/index.html)