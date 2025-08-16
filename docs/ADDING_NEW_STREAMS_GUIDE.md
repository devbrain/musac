# Guide to Adding New Stream Types in Musac

This guide explains how to add new stream types to the musac audio library, using the PC speaker stream implementation as a reference example.

## Overview

Adding a new stream type in musac involves creating:
1. A custom decoder that generates or processes audio data
2. A stream wrapper class that provides the public API
3. Integration with the audio device system

## Architecture Components

### 1. Custom Decoder

The decoder is responsible for generating or decoding audio samples. It must inherit from `musac::decoder`.

**Key Requirements:**
- Inherit from `musac::decoder` base class
- Implement all pure virtual methods
- Handle the `open()` lifecycle properly
- Generate audio in the `do_decode()` method

**Example Structure (from pc_speaker_decoder.cc):**

```cpp
class pc_speaker_decoder : public decoder {
public:
    pc_speaker_decoder();
    
    // Required overrides
    void open(io_stream* stream) override;
    bool is_open() const override;
    std::chrono::microseconds duration() const override;
    bool rewind() override;
    bool seek_to_time(std::chrono::microseconds pos) override;

protected:
    // Audio generation happens here
    size_t do_decode(float buf[], size_t len, bool& call_again, 
                     channels_t device_channels) override;

private:
    // Your custom state
    struct tone {
        float frequency_hz;
        size_t duration_samples;
        size_t start_sample;
    };
    
    std::deque<tone> m_tone_queue;
    float m_phase_accumulator = 0.0f;
    // ... other members
};
```

### 2. Stream Wrapper Class

The stream wrapper provides the public API and manages the underlying `audio_stream`.

**Key Components:**
- Contains an `audio_stream` member
- Provides domain-specific methods
- Handles the decoder lifecycle

**Example Structure (from pc_speaker_stream.hh):**

```cpp
class pc_speaker_stream {
public:
    pc_speaker_stream();
    
    // Domain-specific API
    void sound(float frequency_hz, std::chrono::milliseconds duration);
    void beep(float frequency_hz = 1000.0f);
    void silence(std::chrono::milliseconds duration);
    
    // Delegate audio_stream methods
    void open();
    bool play(unsigned int iterations = 1);
    void stop();
    bool is_playing() const;
    // ... other delegated methods

private:
    audio_stream m_stream;
    std::shared_ptr<pc_speaker_decoder> m_decoder;
};
```

### 3. Audio Device Integration

Add a factory method to `audio_device` to create your stream type.

**In audio_device.hh:**
```cpp
class audio_device {
public:
    // ... existing methods
    
    // Add your factory method
    your_stream_type create_your_stream();
};
```

**In audio_device.cc:**
```cpp
your_stream_type audio_device::create_your_stream() {
    // CRITICAL: Set up audio device data
    audio_device_data aud;
    aud.m_audio_spec = m_pimpl->spec;
    aud.m_frame_size = 4096;
    aud.m_sample_converter = get_from_float_converter(m_pimpl->spec.format);
    
    // Calculate bytes per sample
    switch (m_pimpl->spec.format) {
        case audio_format::u8:
        case audio_format::s8:
            aud.m_bytes_per_sample = 1;
            break;
        case audio_format::s16le:
        case audio_format::s16be:
            aud.m_bytes_per_sample = 2;
            break;
        case audio_format::s32le:
        case audio_format::s32be:
        case audio_format::f32le:
        case audio_format::f32be:
            aud.m_bytes_per_sample = 4;
            break;
        default:
            aud.m_bytes_per_sample = 2;
            break;
    }
    aud.m_bytes_per_frame = aud.m_bytes_per_sample * m_pimpl->spec.channels;
    aud.m_ms_per_frame = 1000.0f / static_cast<float>(m_pimpl->spec.freq);
    
    audio_stream::set_audio_device_data(aud);
    
    // CRITICAL: Register the audio callback
    if (!m_pimpl->stream) {
        create_stream_with_callback(
            [](void* userdata, uint8_t* stream, int len) {
                audio_stream::audio_callback(stream, static_cast<unsigned int>(len));
            },
            nullptr
        );
    }
    
    return your_stream_type();
}
```

## Implementation Steps

### Step 1: Create the Decoder

1. Create header and source files for your decoder
2. Inherit from `musac::decoder`
3. Implement all required virtual methods
4. Focus on the `do_decode()` method for audio generation/processing

### Step 2: Create the Stream Wrapper

1. Create public header in `include/musac/`
2. Create implementation in `src/musac/`
3. Design your domain-specific API
4. Create the internal `audio_stream` with proper `audio_source`

### Step 3: Handle the IO Stream

For generated audio (like PC speaker), create a dummy IO stream:

```cpp
class dummy_io_stream : public io_stream {
public:
    int64_t read(void*, int64_t) override { return 0; }
    int64_t write(const void*, int64_t) override { return 0; }
    int64_t seek(int64_t, int) override { return 0; }
    int64_t tell() override { return 0; }
    int64_t size() override { return 0; }
    bool eof() override { return false; }
};
```

### Step 4: Wire Everything Together

In your stream constructor:

```cpp
your_stream::your_stream() {
    // Create decoder
    m_decoder = std::make_shared<your_decoder>();
    
    // Create dummy IO stream if needed
    auto dummy_stream = std::make_unique<dummy_io_stream>();
    
    // Create audio source with decoder
    audio_source source(m_decoder, std::move(dummy_stream));
    
    // Initialize audio_stream
    m_stream = audio_stream(std::move(source));
}
```

### Step 5: Add to Build System

Update `CMakeLists.txt` to include your new files:

```cmake
set(MUSAC_SOURCES
    # ... existing sources
    src/musac/your_stream.cc
    src/musac/your_decoder.cc
)

set(MUSAC_PUBLIC_HEADERS
    # ... existing headers
    include/musac/your_stream.hh
)
```

## Common Pitfalls and Solutions

### 1. No Audio Output

**Problem:** Stream plays but no sound is produced.

**Solution:** Ensure the audio device callback is registered:
- The `create_your_stream()` method in `audio_device` MUST set up `audio_device_data`
- The `create_your_stream()` method MUST call `create_stream_with_callback()`

### 2. Decoder Not Called

**Problem:** The `do_decode()` method is never invoked.

**Solution:** 
- Ensure `is_open()` returns true after `open()` is called
- The stream must be added to the mixer via `play()`
- Check that the audio device is resumed with `device.resume()`

### 3. Threading Issues

**Problem:** Crashes or race conditions when accessing decoder state.

**Solution:**
- The `do_decode()` method is called from the audio thread
- Use appropriate synchronization (mutex, atomic) for shared state
- Keep audio callback code lock-free where possible

### 4. Format Conversion Issues

**Problem:** Audio sounds distorted or wrong pitch.

**Solution:**
- Generate audio at the correct sample rate (from `m_sample_rate`)
- Handle multi-channel output correctly in `do_decode()`
- Output samples as floating-point in range [-1.0, 1.0]

## Testing Your Stream

Create a test program:

```cpp
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/your_stream.hh>

int main() {
    // Initialize audio system
    auto backend = musac::create_sdl3_backend_v2();
    musac::audio_system::init(backend);
    
    // Open device
    auto device = musac::audio_device::open_default_device(backend);
    device.resume();  // CRITICAL: Start audio processing
    
    // Create and use your stream
    auto stream = device.create_your_stream();
    stream.open();
    
    // Your custom API usage
    stream.do_something();
    
    // Start playback
    stream.play();
    
    // Wait for completion
    while (stream.is_playing()) {
        std::this_thread::sleep_for(100ms);
    }
    
    // Cleanup
    musac::audio_system::done();
    return 0;
}
```

## Debugging Tips

1. **Add debug output** in key methods:
   - `open()` - Confirm the decoder is opened
   - `do_decode()` - Verify it's being called
   - `audio_callback()` - Check the callback is running

2. **Check the audio chain**:
   - Device is created and resumed
   - Stream is opened and played
   - Decoder is opened and generating samples
   - Samples are in correct format and range

3. **Use the Null backend** for testing without audio hardware:
   ```cpp
   auto backend = musac::create_null_backend_v2();
   ```

## Example: PC Speaker Stream

The PC speaker stream is a complete example that demonstrates:
- Custom decoder that generates square waves
- Queue-based tone management
- Thread-safe operation
- Clean API design

See these files for reference:
- `include/musac/pc_speaker_stream.hh` - Public API
- `src/musac/pc_speaker_stream.cc` - Stream implementation
- `src/musac/pc_speaker_decoder.cc` - Decoder implementation
- `example/pc_speaker_example.cc` - Usage example

## Summary

To add a new stream type:
1. Create a decoder inheriting from `musac::decoder`
2. Create a stream wrapper with your custom API
3. Add factory method to `audio_device` with proper initialization
4. Test thoroughly with both SDL3 and Null backends

Remember: The audio device callback registration is critical for audio output!