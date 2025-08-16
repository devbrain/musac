# PC Speaker Stream Implementation Design

## Overview

This document describes the design and implementation of a PC speaker-style audio stream for the musac library. The stream generates square wave tones directly without requiring any decoder, mimicking the behavior of classic PC speakers.

## Requirements

### Functional Requirements
1. **Direct tone generation**: Generate square waves at specified frequencies
2. **Time-based playback**: Play tones for specified durations
3. **No decoder dependency**: Generate audio samples directly
4. **Queueing support**: Queue multiple tones to play in sequence
5. **Real-time control**: Start, stop, pause like regular audio streams
6. **Volume control**: Support standard musac volume controls

### API Requirements
Primary method:
```cpp
void sound(float frequency_hz, std::chrono::milliseconds duration);
```

Additional methods:
```cpp
void beep(float frequency_hz = 1000.0f);  // Short beep (100ms default)
void silence(std::chrono::milliseconds duration);  // Silence period
void clear_queue();  // Clear pending sounds
bool is_queue_empty() const;  // Check if queue is empty
```

## Architecture

### Class Hierarchy

```
audio_stream (base)
    └── pc_speaker_stream
            ├── pc_speaker_source (internal audio_source)
            └── tone_queue (internal queue management)
```

### Key Components

#### 1. `pc_speaker_stream` (Public Interface)
- Inherits from `audio_stream`
- Manages tone queue
- Provides PC speaker-specific methods
- Handles thread safety for queue operations

#### 2. `pc_speaker_source` (Internal Audio Generator)
- Inherits from `audio_source`
- Generates square wave samples
- Processes tone queue
- No decoder or resampler needed

#### 3. `tone_queue` (Internal Queue)
- Thread-safe queue of tone commands
- Supports frequency/duration pairs
- Handles silence periods

## Implementation Details

### Square Wave Generation

The square wave generator uses a simple phase accumulator approach:

```cpp
class square_wave_generator {
    float phase = 0.0f;
    float phase_increment = 0.0f;
    
    void set_frequency(float hz, float sample_rate) {
        phase_increment = (2.0f * hz) / sample_rate;
    }
    
    float generate_sample() {
        phase += phase_increment;
        if (phase >= 1.0f) phase -= 2.0f;
        return (phase >= 0.0f) ? 0.5f : -0.5f;  // Square wave
    }
};
```

### Tone Queue Structure

```cpp
struct tone_command {
    float frequency_hz;  // 0 = silence
    std::chrono::milliseconds duration;
    std::chrono::steady_clock::time_point queued_at;
};

class tone_queue {
    std::deque<tone_command> commands;
    mutable std::mutex mutex;
    tone_command current_tone;
    size_t samples_remaining;
};
```

### Audio Generation Flow

1. `pc_speaker_source::read_samples()` is called by the audio callback
2. Check if current tone has samples remaining
3. If not, dequeue next tone from queue
4. Generate square wave samples at specified frequency
5. Fill buffer with generated samples
6. Handle stereo by duplicating mono signal

### Thread Safety

- Tone queue protected by mutex
- Queue operations are lock-free from audio callback perspective
- Uses double-buffering for current tone to avoid locks in audio path

## File Structure

```
include/musac/
├── pc_speaker_stream.hh     # Public API header

src/musac/
├── pc_speaker_stream.cc      # Main implementation
├── pc_speaker_source.hh      # Internal source header
└── pc_speaker_source.cc      # Source implementation
```

## Usage Examples

### Basic Usage
```cpp
#include <musac/pc_speaker_stream.hh>
#include <musac/audio_system.hh>

// Create and play a simple beep
auto speaker = audio_system::new_pc_speaker_stream();
speaker.sound(440.0f, 500ms);  // A4 note for 500ms
speaker.play();
```

### Playing a Melody
```cpp
// Play a simple scale
speaker.sound(261.63f, 200ms);  // C4
speaker.sound(293.66f, 200ms);  // D4
speaker.sound(329.63f, 200ms);  // E4
speaker.sound(349.23f, 200ms);  // F4
speaker.sound(392.00f, 200ms);  // G4
speaker.play();
```

### Morse Code Example
```cpp
void morse_dot(pc_speaker_stream& s) {
    s.sound(800.0f, 100ms);
    s.silence(100ms);
}

void morse_dash(pc_speaker_stream& s) {
    s.sound(800.0f, 300ms);
    s.silence(100ms);
}

// SOS: ... --- ...
for(int i = 0; i < 3; i++) morse_dot(speaker);
s.silence(200ms);
for(int i = 0; i < 3; i++) morse_dash(speaker);
s.silence(200ms);
for(int i = 0; i < 3; i++) morse_dot(speaker);
```

## Special Considerations

### Sample Rate Independence
The generator must work correctly at any sample rate (22050, 44100, 48000, etc.)

### Frequency Limits
- Minimum frequency: 20 Hz (below audible range)
- Maximum frequency: sample_rate / 2 (Nyquist limit)
- Typical PC speaker range: 37 Hz - 32767 Hz

### Click Prevention
To prevent clicking:
- Apply short fade-in/out (1-2ms) at tone boundaries
- Zero-crossing detection for smooth transitions
- Optional: Use band-limited square waves (additive synthesis)

### Memory Management
- Pre-allocate queue to avoid allocations in audio callback
- Use ring buffer for lock-free queue implementation
- Recycle tone_command objects

## Testing Strategy

### Unit Tests
1. Square wave generation accuracy
2. Frequency accuracy verification
3. Duration timing tests
4. Queue operations (add, clear, overflow)
5. Thread safety stress tests

### Integration Tests
1. Play through audio system
2. Volume control verification
3. Pause/resume functionality
4. Multiple simultaneous streams
5. Device switching behavior

### Golden Data Tests
Generate known patterns and compare output:
- Single frequency tones
- Frequency sweeps
- Silence periods
- Complex sequences

## Performance Considerations

### CPU Usage
- Square wave generation: ~0.1% CPU per stream
- No complex DSP operations
- Minimal memory bandwidth

### Latency
- Near-zero latency for tone generation
- Queue processing adds < 1ms
- Suitable for real-time applications

### Memory Usage
- Fixed ~4KB per stream
- Queue pre-allocated (default 1000 commands)
- No dynamic allocations during playback

## Future Enhancements

### Phase 2 Features
- Waveform selection (triangle, sawtooth, sine)
- Duty cycle control for PWM effects
- Frequency modulation/sweeps
- ADSR envelope support

### Phase 3 Features
- DTMF tone generation
- Musical note names (C4, A#5, etc.)
- Tempo/BPM support
- Simple scripting language

## Implementation Checklist

- [ ] Create pc_speaker_source class
- [ ] Implement square wave generator
- [ ] Implement tone queue with thread safety
- [ ] Create pc_speaker_stream public interface
- [ ] Add sound() and helper methods
- [ ] Implement audio_source interface
- [ ] Add click prevention
- [ ] Write unit tests
- [ ] Write integration tests
- [ ] Add documentation and examples
- [ ] Performance profiling
- [ ] Code review and cleanup