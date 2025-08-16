# Playback Control Guide

This guide covers all aspects of controlling audio playback in Musac, including play/pause/stop operations, fade effects, state management, and callbacks.

## Table of Contents

1. [Basic Playback Control](#basic-playback-control)
2. [Fade In/Out Effects](#fade-inout-effects)
3. [State Management](#state-management)
4. [Callbacks and Events](#callbacks-and-events)
5. [Advanced Control Patterns](#advanced-control-patterns)
6. [Thread Safety](#thread-safety)
7. [Best Practices](#best-practices)

## Basic Playback Control

### Playing Audio

The `play()` method starts or resumes audio playback:

```cpp
// Simple play
stream->play();

// Play with loop count
stream->play(3);  // Play 3 times
stream->play(musac::infinite_loop);  // Loop forever

// Play with fade-in
stream->play(1, std::chrono::seconds(2));  // Fade in over 2 seconds
```

### Pausing and Resuming

Pause temporarily stops playback while maintaining position:

```cpp
// Pause playback
stream->pause();

// Check if paused
if (stream->is_paused()) {
    std::cout << "Stream is paused\n";
}

// Resume from pause
stream->resume();
```

**Important**: Only paused streams can be resumed. Stopped streams must be rewound and played again.

### Stopping

Stop terminates playback and resets to the beginning:

```cpp
// Immediate stop
stream->stop();

// Stop with fade-out
stream->stop(std::chrono::milliseconds(500));  // 500ms fade-out
```

### Complete Example

```cpp
#include <musac/audio_device.hh>
#include <musac/audio_loader.hh>
#include <thread>
#include <chrono>

void playback_control_demo(const std::string& file) {
    musac::audio_device device;
    device.open_default_device();
    
    auto source = musac::audio_loader::load(file);
    auto stream = device.create_stream(std::move(source));
    
    // Start playback
    std::cout << "Playing...\n";
    stream->play();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Pause
    std::cout << "Pausing...\n";
    stream->pause();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Resume
    std::cout << "Resuming...\n";
    stream->resume();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Stop with fade-out
    std::cout << "Stopping with fade...\n";
    stream->stop(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Play again with fade-in
    std::cout << "Playing with fade-in...\n";
    stream->play(1, std::chrono::seconds(2));
}
```

## Fade In/Out Effects

### Fade-In on Play

Gradually increase volume from 0 to target volume:

```cpp
// Set target volume
stream->set_volume(0.8f);

// Play with 3-second fade-in
stream->play(1, std::chrono::seconds(3));
```

### Fade-Out on Stop

Gradually decrease volume to 0 before stopping:

```cpp
// Stop with 1-second fade-out
stream->stop(std::chrono::seconds(1));

// The fade duration can be customized
stream->stop(std::chrono::milliseconds(250));  // Quick fade
stream->stop(std::chrono::seconds(5));          // Slow fade
```

### Custom Fade Implementation

For more control over fading:

```cpp
class FadeController {
private:
    musac::audio_stream* m_stream;
    float m_start_volume;
    float m_target_volume;
    std::chrono::steady_clock::time_point m_fade_start;
    std::chrono::milliseconds m_fade_duration;
    bool m_fading = false;
    
public:
    void start_fade(musac::audio_stream* stream,
                    float target_volume,
                    std::chrono::milliseconds duration) {
        m_stream = stream;
        m_start_volume = stream->get_volume();
        m_target_volume = target_volume;
        m_fade_duration = duration;
        m_fade_start = std::chrono::steady_clock::now();
        m_fading = true;
    }
    
    void update() {
        if (!m_fading) return;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                      (now - m_fade_start);
        
        if (elapsed >= m_fade_duration) {
            m_stream->set_volume(m_target_volume);
            m_fading = false;
            return;
        }
        
        float progress = static_cast<float>(elapsed.count()) / 
                        m_fade_duration.count();
        float volume = m_start_volume + 
                      (m_target_volume - m_start_volume) * progress;
        m_stream->set_volume(volume);
    }
    
    bool is_fading() const { return m_fading; }
};
```

### Cross-Fade Between Tracks

```cpp
void cross_fade(musac::audio_stream* from_stream,
                musac::audio_stream* to_stream,
                std::chrono::milliseconds duration) {
    // Start the new track at volume 0
    to_stream->set_volume(0.0f);
    to_stream->play();
    
    // Get original volumes
    float from_volume = from_stream->get_volume();
    float to_volume = 1.0f;  // Target volume for new track
    
    // Perform cross-fade
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                      (now - start);
        
        if (elapsed >= duration) break;
        
        float progress = static_cast<float>(elapsed.count()) / 
                        duration.count();
        
        from_stream->set_volume(from_volume * (1.0f - progress));
        to_stream->set_volume(to_volume * progress);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Final state
    from_stream->stop();
    to_stream->set_volume(to_volume);
}
```

## State Management

### Stream States

Audio streams can be in one of several states:

```cpp
enum class stream_state {
    created,    // Stream created but not started
    playing,    // Currently playing
    paused,     // Temporarily paused
    stopped,    // Stopped (at beginning)
    finished    // Playback completed
};
```

### Checking State

```cpp
// Primary state checks
bool is_playing = stream->is_playing();
bool is_paused = stream->is_paused();

// Derived state checks
bool is_stopped = !is_playing && !is_paused;
bool is_active = is_playing || is_paused;

// Position check
auto position = stream->tell_time();
bool at_beginning = (position.count() == 0);
```

### State Machine Example

```cpp
class AudioStateMachine {
public:
    enum State {
        IDLE,
        PLAYING,
        PAUSED,
        FADING_IN,
        FADING_OUT
    };
    
private:
    State m_state = IDLE;
    musac::audio_stream* m_stream;
    
public:
    AudioStateMachine(musac::audio_stream* stream) 
        : m_stream(stream) {}
    
    void play() {
        switch (m_state) {
            case IDLE:
            case PAUSED:
                m_stream->play();
                m_state = PLAYING;
                break;
            case PLAYING:
            case FADING_IN:
                // Already playing
                break;
            case FADING_OUT:
                // Cancel fade and resume
                m_stream->set_volume(1.0f);
                m_state = PLAYING;
                break;
        }
    }
    
    void pause() {
        if (m_state == PLAYING || m_state == FADING_IN) {
            m_stream->pause();
            m_state = PAUSED;
        }
    }
    
    void stop() {
        if (m_state != IDLE) {
            m_stream->stop();
            m_state = IDLE;
        }
    }
    
    void fade_in(std::chrono::milliseconds duration) {
        if (m_state == IDLE) {
            m_stream->play(1, duration);
            m_state = FADING_IN;
            // Set timer to transition to PLAYING
        }
    }
    
    void fade_out(std::chrono::milliseconds duration) {
        if (m_state == PLAYING) {
            m_stream->stop(duration);
            m_state = FADING_OUT;
            // Set timer to transition to IDLE
        }
    }
    
    State get_state() const { return m_state; }
};
```

## Callbacks and Events

### Setting Up Callbacks

Musac supports callbacks for various playback events:

```cpp
// Playback finished callback
stream->set_finished_callback([](musac::audio_stream* stream) {
    std::cout << "Playback finished\n";
});

// Loop callback (called at end of each loop)
stream->set_loop_callback([](musac::audio_stream* stream, int loops_remaining) {
    std::cout << "Loop completed, " << loops_remaining << " remaining\n";
});

// Position callback (called periodically)
stream->set_position_callback([](musac::audio_stream* stream, 
                                 std::chrono::microseconds position) {
    auto seconds = position.count() / 1000000.0;
    std::cout << "Position: " << seconds << " seconds\n";
}, std::chrono::milliseconds(100));  // Update every 100ms
```

### Event-Driven Playback

```cpp
class MusicPlayer {
private:
    musac::audio_device m_device;
    std::unique_ptr<musac::audio_stream> m_current_stream;
    std::queue<std::string> m_playlist;
    
public:
    MusicPlayer() {
        m_device.open_default_device();
    }
    
    void add_to_playlist(const std::string& file) {
        m_playlist.push(file);
    }
    
    void start() {
        play_next();
    }
    
private:
    void play_next() {
        if (m_playlist.empty()) {
            std::cout << "Playlist finished\n";
            return;
        }
        
        std::string file = m_playlist.front();
        m_playlist.pop();
        
        auto source = musac::audio_loader::load(file);
        m_current_stream = m_device.create_stream(std::move(source));
        
        // Set up callback for when track finishes
        m_current_stream->set_finished_callback(
            [this](musac::audio_stream*) {
                this->play_next();
            }
        );
        
        std::cout << "Playing: " << file << "\n";
        m_current_stream->play();
    }
};
```

### Custom Event System

```cpp
#include <functional>
#include <vector>

class AudioEventDispatcher {
public:
    enum EventType {
        STARTED,
        PAUSED,
        RESUMED,
        STOPPED,
        FINISHED,
        LOOP_POINT,
        ERROR
    };
    
    using EventHandler = std::function<void(EventType, musac::audio_stream*)>;
    
private:
    std::vector<EventHandler> m_handlers;
    musac::audio_stream* m_stream;
    
public:
    void add_handler(EventHandler handler) {
        m_handlers.push_back(handler);
    }
    
    void dispatch(EventType event) {
        for (auto& handler : m_handlers) {
            handler(event, m_stream);
        }
    }
    
    void monitor_stream(musac::audio_stream* stream) {
        m_stream = stream;
        
        // Set up stream callbacks
        stream->set_finished_callback([this](musac::audio_stream*) {
            dispatch(FINISHED);
        });
        
        stream->set_loop_callback([this](musac::audio_stream*, int) {
            dispatch(LOOP_POINT);
        });
    }
};

// Usage
AudioEventDispatcher dispatcher;

dispatcher.add_handler([](AudioEventDispatcher::EventType event, 
                         musac::audio_stream* stream) {
    switch (event) {
        case AudioEventDispatcher::STARTED:
            std::cout << "Playback started\n";
            break;
        case AudioEventDispatcher::FINISHED:
            std::cout << "Playback finished\n";
            break;
        // ... handle other events
    }
});
```

## Advanced Control Patterns

### Gapless Playback

For seamless track transitions:

```cpp
class GaplessPlayer {
private:
    musac::audio_device m_device;
    std::unique_ptr<musac::audio_stream> m_current;
    std::unique_ptr<musac::audio_stream> m_next;
    
public:
    void prepare_next(const std::string& file) {
        auto source = musac::audio_loader::load(file);
        m_next = m_device.create_stream(std::move(source));
        // Pre-buffer but don't play yet
    }
    
    void switch_tracks() {
        if (!m_next) return;
        
        // Start next track immediately
        m_next->play();
        
        // Stop current without fade
        if (m_current) {
            m_current->stop();
        }
        
        // Swap
        m_current = std::move(m_next);
        m_next.reset();
    }
};
```

### Duck Audio (Reduce volume temporarily)

```cpp
class AudioDucker {
private:
    std::vector<musac::audio_stream*> m_music_streams;
    float m_normal_volume = 1.0f;
    float m_ducked_volume = 0.3f;
    bool m_ducked = false;
    
public:
    void add_music_stream(musac::audio_stream* stream) {
        m_music_streams.push_back(stream);
    }
    
    void duck() {
        if (m_ducked) return;
        
        for (auto stream : m_music_streams) {
            stream->set_volume(m_ducked_volume);
        }
        m_ducked = true;
    }
    
    void unduck() {
        if (!m_ducked) return;
        
        for (auto stream : m_music_streams) {
            stream->set_volume(m_normal_volume);
        }
        m_ducked = false;
    }
    
    void play_important_sound(musac::audio_stream* sound) {
        duck();
        
        sound->set_finished_callback([this](musac::audio_stream*) {
            this->unduck();
        });
        
        sound->play();
    }
};
```

### Synchronized Playback

Start multiple streams at exactly the same time:

```cpp
class SyncPlayer {
private:
    std::vector<musac::audio_stream*> m_streams;
    
public:
    void add_stream(musac::audio_stream* stream) {
        m_streams.push_back(stream);
    }
    
    void play_all() {
        // Pause all first (prepare them)
        for (auto stream : m_streams) {
            stream->play();
            stream->pause();
        }
        
        // Resume all at once
        for (auto stream : m_streams) {
            stream->resume();
        }
    }
    
    void stop_all() {
        for (auto stream : m_streams) {
            stream->stop();
        }
    }
};
```

## Thread Safety

### Thread-Safe Operations

All control methods are thread-safe:

```cpp
// Safe to call from any thread
stream->play();
stream->pause();
stream->stop();
stream->set_volume(0.5f);
stream->is_playing();
```

### Callback Thread Context

Callbacks are called from the audio thread:

```cpp
stream->set_finished_callback([](musac::audio_stream* stream) {
    // This runs on the audio thread
    // Keep it short and don't block!
    
    // Good: Set a flag
    finished_flag.store(true);
    
    // Bad: Heavy processing
    // process_large_file();  // Don't do this!
});
```

### Thread-Safe Event Queue

```cpp
#include <queue>
#include <mutex>
#include <condition_variable>

class ThreadSafeEventQueue {
public:
    struct Event {
        enum Type { PLAY, PAUSE, STOP, VOLUME_CHANGE };
        Type type;
        float value;  // For volume changes
    };
    
private:
    std::queue<Event> m_events;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    
public:
    void push(const Event& event) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_events.push(event);
        }
        m_cv.notify_one();
    }
    
    bool pop(Event& event, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_cv.wait_for(lock, timeout, [this] { 
            return !m_events.empty(); 
        })) {
            event = m_events.front();
            m_events.pop();
            return true;
        }
        return false;
    }
    
    void process_events(musac::audio_stream* stream) {
        Event event;
        while (pop(event, std::chrono::milliseconds(0))) {
            switch (event.type) {
                case Event::PLAY:
                    stream->play();
                    break;
                case Event::PAUSE:
                    stream->pause();
                    break;
                case Event::STOP:
                    stream->stop();
                    break;
                case Event::VOLUME_CHANGE:
                    stream->set_volume(event.value);
                    break;
            }
        }
    }
};
```

## Best Practices

### 1. Always Check State Before Operations

```cpp
// Good
if (!stream->is_playing()) {
    stream->play();
}

// Bad - might restart from beginning
stream->play();  // What if already playing?
```

### 2. Use Appropriate Fade Durations

```cpp
// Short sounds: quick fades
explosion_stream->stop(std::chrono::milliseconds(50));

// Music: longer fades
music_stream->stop(std::chrono::seconds(2));

// Dialog: medium fades
voice_stream->stop(std::chrono::milliseconds(200));
```

### 3. Handle Edge Cases

```cpp
// Check stream validity
if (stream && stream->is_valid()) {
    stream->play();
}

// Handle rapid state changes
void safe_pause(musac::audio_stream* stream) {
    if (stream->is_playing() && !stream->is_paused()) {
        stream->pause();
    }
}
```

### 4. Resource Management

```cpp
class StreamManager {
private:
    std::vector<std::unique_ptr<musac::audio_stream>> m_streams;
    
public:
    void cleanup_finished() {
        m_streams.erase(
            std::remove_if(m_streams.begin(), m_streams.end(),
                [](const auto& stream) {
                    return !stream->is_playing() && !stream->is_paused();
                }),
            m_streams.end()
        );
    }
    
    void stop_all() {
        for (auto& stream : m_streams) {
            if (stream->is_playing()) {
                stream->stop(std::chrono::milliseconds(100));
            }
        }
    }
};
```

### 5. Smooth Volume Changes

```cpp
// Smooth volume ramping
void ramp_volume(musac::audio_stream* stream,
                 float target_volume,
                 std::chrono::milliseconds duration) {
    float start_volume = stream->get_volume();
    float delta = target_volume - start_volume;
    
    const int steps = 20;
    auto step_duration = duration / steps;
    
    for (int i = 1; i <= steps; ++i) {
        float progress = static_cast<float>(i) / steps;
        float volume = start_volume + delta * progress;
        stream->set_volume(volume);
        std::this_thread::sleep_for(step_duration);
    }
}
```

## Troubleshooting

### Stream Won't Play

```cpp
void diagnose_stream(musac::audio_stream* stream) {
    std::cout << "Stream diagnostics:\n";
    std::cout << "  Valid: " << stream->is_valid() << "\n";
    std::cout << "  Playing: " << stream->is_playing() << "\n";
    std::cout << "  Paused: " << stream->is_paused() << "\n";
    std::cout << "  Volume: " << stream->get_volume() << "\n";
    std::cout << "  Position: " << stream->tell_time().count() << "µs\n";
    
    auto duration = stream->duration();
    if (duration.count() > 0) {
        std::cout << "  Duration: " << duration.count() << "µs\n";
    } else {
        std::cout << "  Duration: Infinite/Unknown\n";
    }
}
```

### Callbacks Not Firing

```cpp
// Ensure stream stays alive
class CallbackTester {
    std::unique_ptr<musac::audio_stream> m_stream;  // Keep stream alive
    
public:
    void test() {
        m_stream = create_stream();
        
        m_stream->set_finished_callback([](musac::audio_stream*) {
            std::cout << "Finished!\n";
        });
        
        m_stream->play();
        // Stream stays alive until CallbackTester is destroyed
    }
};
```

## See Also

- [Playing Audio Files](PLAYING_AUDIO_FILES.md)
- [Volume and Panning Guide](VOLUME_AND_PANNING.md)
- [Looping Audio Guide](LOOPING_AUDIO.md)
- [Error Handling Guide](ERROR_HANDLING_GUIDE.md)
- [API Reference](doxygen/html/index.html)