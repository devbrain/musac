# Error Handling Guide for MUSAC

## Overview

MUSAC uses exception-based error handling with the failsafe library. This guide explains the error handling patterns and best practices.

## Exception Hierarchy

```cpp
// Base exception class
class musac_error : public std::runtime_error

// Specific error types
class device_error : public musac_error     // Audio device errors
class format_error : public musac_error     // Format/codec errors  
class decoder_error : public musac_error    // Decoder errors
class codec_error : public musac_error      // Codec errors
class io_error : public musac_error         // I/O errors
class resource_error : public musac_error   // Resource errors
class state_error : public musac_error      // State/logic errors
```

## API Changes (Breaking)

### Methods That Now Throw

These methods changed from returning `bool` to `void` and throw on error:

```cpp
// Old API (returns bool)
if (!stream.open()) {
    // handle error
}

// New API (throws)
try {
    stream.open();  // throws musac_error on failure
} catch (const musac_error& e) {
    // handle error
}
```

Affected methods:
- `audio_stream::open()` - throws on failure to open stream
- `audio_source::open()` - throws on failure to open source
- `decoder::open()` - throws on failure to open decoder
- `audio_backend::init()` - throws on initialization failure
- `audio_device_interface::open_device()` - throws on device open failure
- `audio_device_interface::create_stream()` - throws on stream creation failure

### Methods That Still Return Bool

These methods return `bool` for legitimate failures or capability queries:

```cpp
// Capability checks - may legitimately return false
bool can_rewind = stream.rewind();
bool seeked = stream.seek_to_time(pos);

// State queries - always safe
bool playing = stream.is_playing();
bool paused = stream.is_paused();

// Operations that can fail normally
bool paused = device.pause();
bool resumed = device.resume();
```

## Error Handling Patterns

### Basic Error Handling

```cpp
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/error.hh>

try {
    // Initialize audio system
    if (!audio_system::init()) {
        // System-level initialization failed
        return false;
    }
    
    // Open audio device
    auto device = audio_device::open_default_device();
    
    // Create and open stream
    auto stream = device.create_stream(std::move(source));
    stream.open();  // throws on error
    
    // Start playback
    stream.play();
    
} catch (const musac::device_error& e) {
    // Handle device-specific errors
    std::cerr << "Audio device error: " << e.what() << std::endl;
} catch (const musac::format_error& e) {
    // Handle format/codec errors
    std::cerr << "Format error: " << e.what() << std::endl;
} catch (const musac::musac_error& e) {
    // Handle any other musac errors
    std::cerr << "Audio error: " << e.what() << std::endl;
}
```

### Custom Decoder Implementation

```cpp
class my_decoder : public musac::decoder {
public:
    void open() override {
        // Validate file format
        if (!is_valid_format()) {
            THROW_RUNTIME("Invalid file format");
        }
        
        // Allocate resources
        if (!allocate_buffers()) {
            THROW_RUNTIME("Failed to allocate decoder buffers");
        }
    }
    
    // Capability checks still return bool
    bool rewind() override {
        return m_seekable;  // May legitimately be false
    }
};
```

## Real-time Safety

The audio callback is real-time safe and will NEVER throw exceptions:

```cpp
// In audio_stream::audio_callback
// - No exceptions thrown
// - No memory allocation
// - Lock-free operations only
// - Static logging flags prevent log spam
```

## Error Context

All exceptions thrown include:
- File and line information (automatic via THROW_RUNTIME)
- Contextual error messages
- Nested exception support for error chains

Example error message:
```
Failed to initialize SDL audio subsystem: No available audio device
  at /path/to/sdl3_audio_backend.cc:33
```

## Logger Categories

Different subsystems use different logger categories:
- `musac` - Core library
- `musac::codecs` - Codec implementations  
- `musac::sdk` - SDK components

## Migration Guide

### Before (Bool Returns)
```cpp
audio_source source("file.mp3");
if (!source.open(44100, 2, 1024)) {
    std::cerr << "Failed to open audio source\n";
    return false;
}
```

### After (Exceptions)
```cpp
try {
    audio_source source("file.mp3");
    source.open(44100, 2, 1024);  // throws on error
} catch (const musac::io_error& e) {
    std::cerr << "Failed to open audio source: " << e.what() << "\n";
    return false;
}
```

## Best Practices

1. **Catch specific exceptions** when you can handle them differently
2. **Always catch musac_error** as a fallback for any musac errors
3. **Don't throw in real-time threads** - the audio callback is exception-free
4. **Use THROW_RUNTIME** macro for consistent error reporting
5. **Include context** in error messages (what failed, why, actual values)
6. **Check capabilities** with bool returns before attempting operations

## Thread Safety

- All error handling is thread-safe
- Exceptions can be thrown from any non-real-time thread
- The audio callback will never throw (real-time safe)
- Error state is not shared between threads