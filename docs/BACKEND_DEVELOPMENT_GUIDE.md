# Backend Development Guide

## Overview

This guide explains how to develop custom audio backends for the musac library. Backends provide the interface between musac's high-level audio system and platform-specific audio APIs like SDL, ALSA, CoreAudio, WASAPI, etc.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Backend Interface](#backend-interface)
3. [Implementation Steps](#implementation-steps)
4. [Example: Minimal Backend](#example-minimal-backend)
5. [Example: Full-Featured Backend](#example-full-featured-backend)
6. [Testing Your Backend](#testing-your-backend)
7. [Common Patterns](#common-patterns)
8. [Platform Integration](#platform-integration)
9. [Best Practices](#best-practices)

## Architecture Overview

### Backend Responsibilities

Audio backends in musac are responsible for:

- **Device Management**: Enumerating and opening audio devices
- **Format Negotiation**: Converting between musac and platform formats
- **Audio Callbacks**: Managing the audio thread and callbacks
- **Stream Creation**: Creating audio streams for data playback
- **Resource Management**: Proper cleanup and error handling

### Backend Lifecycle

```
1. Backend Creation
   ↓
2. init() - Initialize audio subsystem
   ↓
3. enumerate_devices() - Discover available devices
   ↓
4. open_device() - Open specific device
   ↓
5. create_stream() - Create audio streams
   ↓
6. Audio playback via callbacks
   ↓
7. close_device() - Close device
   ↓
8. shutdown() - Cleanup subsystem
```

## Backend Interface

All backends must inherit from `audio_backend` and implement the required virtual methods:

```cpp
#include <musac/sdk/audio_backend.hh>

class my_backend : public musac::audio_backend {
public:
    // Lifecycle management
    void init() override;
    void shutdown() override;
    std::string get_name() const override;
    bool is_initialized() const override;
    
    // Device enumeration
    std::vector<device_info> enumerate_devices(bool playback) override;
    device_info get_default_device(bool playback) override;
    
    // Device management
    uint32_t open_device(const std::string& device_id,
                        const audio_spec& spec,
                        audio_spec& obtained_spec) override;
    void close_device(uint32_t device_handle) override;
    
    // Device properties
    audio_format get_device_format(uint32_t device_handle) override;
    sample_rate_t get_device_frequency(uint32_t device_handle) override;
    channels_t get_device_channels(uint32_t device_handle) override;
    float get_device_gain(uint32_t device_handle) override;
    void set_device_gain(uint32_t device_handle, float gain) override;
    
    // Device control
    bool pause_device(uint32_t device_handle) override;
    bool resume_device(uint32_t device_handle) override;
    bool is_device_paused(uint32_t device_handle) override;
    
    // Stream creation
    std::unique_ptr<audio_stream_interface> create_stream(
        uint32_t device_handle,
        const audio_spec& spec,
        void (*callback)(void* userdata, uint8_t* stream, int len),
        void* userdata) override;
    
    // Capabilities
    bool supports_recording() const override;
    int get_max_open_devices() const override;
};
```

## Implementation Steps

### Step 1: Create Backend Class

```cpp
// my_backend.hh
#pragma once
#include <musac/sdk/audio_backend.hh>
#include <map>
#include <mutex>

class my_backend : public musac::audio_backend {
private:
    struct device_context {
        // Platform-specific device handle
        void* native_handle;
        audio_spec spec;
        bool is_paused;
        float gain;
    };
    
    bool m_initialized = false;
    std::map<uint32_t, device_context> m_devices;
    uint32_t m_next_handle = 1;
    mutable std::mutex m_mutex;
    
public:
    // ... interface implementation
};
```

### Step 2: Implement Initialization

```cpp
void my_backend::init() {
    if (m_initialized) {
        return;
    }
    
    // Initialize platform API
    if (!platform_audio_init()) {
        throw std::runtime_error("Failed to initialize audio subsystem");
    }
    
    m_initialized = true;
}

void my_backend::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    // Close all open devices
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [handle, context] : m_devices) {
        platform_close_device(context.native_handle);
    }
    m_devices.clear();
    
    // Shutdown platform API
    platform_audio_shutdown();
    m_initialized = false;
}
```

### Step 3: Implement Device Enumeration

```cpp
std::vector<device_info> my_backend::enumerate_devices(bool playback) {
    std::vector<device_info> devices;
    
    // Query platform for devices
    int count = platform_get_device_count(playback);
    for (int i = 0; i < count; ++i) {
        auto* info = platform_get_device_info(i, playback);
        if (info) {
            devices.push_back({
                .name = info->name,
                .id = std::to_string(i),
                .is_default = (i == 0),
                .channels = info->channels,
                .sample_rate = info->sample_rate
            });
        }
    }
    
    return devices;
}
```

### Step 4: Implement Device Opening

```cpp
uint32_t my_backend::open_device(const std::string& device_id,
                                 const audio_spec& spec,
                                 audio_spec& obtained_spec) {
    if (!m_initialized) {
        throw std::runtime_error("Backend not initialized");
    }
    
    // Parse device ID (empty = default)
    int device_index = device_id.empty() ? 0 : std::stoi(device_id);
    
    // Convert musac spec to platform format
    platform_audio_spec platform_spec;
    platform_spec.freq = spec.freq;
    platform_spec.format = convert_format(spec.format);
    platform_spec.channels = spec.channels;
    platform_spec.samples = spec.samples;
    
    // Open platform device
    platform_audio_spec obtained_platform;
    void* handle = platform_open_device(
        device_index, 
        &platform_spec, 
        &obtained_platform
    );
    
    if (!handle) {
        throw std::runtime_error("Failed to open audio device");
    }
    
    // Convert back to musac format
    obtained_spec.freq = obtained_platform.freq;
    obtained_spec.format = convert_format_back(obtained_platform.format);
    obtained_spec.channels = obtained_platform.channels;
    obtained_spec.samples = obtained_platform.samples;
    
    // Store device context
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t device_handle = m_next_handle++;
    m_devices[device_handle] = {
        .native_handle = handle,
        .spec = obtained_spec,
        .is_paused = false,
        .gain = 1.0f
    };
    
    return device_handle;
}
```

### Step 5: Implement Stream Creation

```cpp
class my_audio_stream : public audio_stream_interface {
private:
    void* m_native_stream;
    audio_spec m_spec;
    
public:
    my_audio_stream(void* device, const audio_spec& spec,
                   void (*callback)(void*, uint8_t*, int),
                   void* userdata) 
        : m_spec(spec) {
        // Create platform stream
        m_native_stream = platform_create_stream(
            device, 
            &spec,
            callback,
            userdata
        );
        if (!m_native_stream) {
            throw std::runtime_error("Failed to create stream");
        }
    }
    
    ~my_audio_stream() {
        platform_destroy_stream(m_native_stream);
    }
    
    // Implement stream interface...
};

std::unique_ptr<audio_stream_interface> 
my_backend::create_stream(uint32_t device_handle,
                         const audio_spec& spec,
                         void (*callback)(void*, uint8_t*, int),
                         void* userdata) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_devices.find(device_handle);
    if (it == m_devices.end()) {
        throw std::runtime_error("Invalid device handle");
    }
    
    return std::make_unique<my_audio_stream>(
        it->second.native_handle,
        spec,
        callback,
        userdata
    );
}
```

## Example: Minimal Backend

Here's a complete minimal backend implementation:

```cpp
// null_backend.cc - Silent backend for testing
#include "null_backend.hh"
#include <chrono>
#include <thread>

class null_stream : public audio_stream_interface {
private:
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    audio_spec m_spec;
    void (*m_callback)(void*, uint8_t*, int);
    void* m_userdata;
    
    void audio_thread() {
        const int buffer_size = m_spec.samples * m_spec.channels * 
                              get_sample_size(m_spec.format);
        std::vector<uint8_t> buffer(buffer_size, 0);
        
        while (m_running) {
            if (m_callback) {
                m_callback(m_userdata, buffer.data(), buffer_size);
            }
            
            // Simulate audio timing
            auto duration = std::chrono::microseconds(
                1000000 * m_spec.samples / m_spec.freq
            );
            std::this_thread::sleep_for(duration);
        }
    }
    
public:
    null_stream(const audio_spec& spec,
               void (*callback)(void*, uint8_t*, int),
               void* userdata)
        : m_spec(spec), m_callback(callback), m_userdata(userdata) {
        m_running = true;
        m_thread = std::thread(&null_stream::audio_thread, this);
    }
    
    ~null_stream() {
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
};

// Backend implementation
void null_backend::init() {
    m_initialized = true;
}

void null_backend::shutdown() {
    m_devices.clear();
    m_initialized = false;
}

std::vector<device_info> null_backend::enumerate_devices(bool) {
    return {{
        .name = "Null Device",
        .id = "null",
        .is_default = true,
        .channels = 2,
        .sample_rate = 48000
    }};
}

uint32_t null_backend::open_device(const std::string&,
                                   const audio_spec& spec,
                                   audio_spec& obtained) {
    obtained = spec;  // Accept any format
    uint32_t handle = m_next_handle++;
    m_devices[handle] = {spec, false, 1.0f};
    return handle;
}

// ... implement remaining methods
```

## Example: Full-Featured Backend

See the SDL3 backend implementation in `src/backends/sdl3/` for a complete production backend with:

- Full device enumeration
- Hot-plug device support
- Hardware volume control
- Low-latency audio callbacks
- Format conversion
- Error recovery

## Testing Your Backend

### Unit Tests

```cpp
TEST_CASE("Backend initialization") {
    auto backend = std::make_unique<my_backend>();
    
    REQUIRE_NOTHROW(backend->init());
    REQUIRE(backend->is_initialized());
    
    REQUIRE_NOTHROW(backend->shutdown());
    REQUIRE_FALSE(backend->is_initialized());
}

TEST_CASE("Device enumeration") {
    auto backend = std::make_unique<my_backend>();
    backend->init();
    
    auto devices = backend->enumerate_devices(true);
    REQUIRE_FALSE(devices.empty());
    
    // Should have at least one default device
    auto default_found = std::any_of(devices.begin(), devices.end(),
        [](const device_info& d) { return d.is_default; });
    REQUIRE(default_found);
}

TEST_CASE("Audio playback") {
    auto backend = std::make_unique<my_backend>();
    backend->init();
    
    audio_spec desired{48000, audio_format::f32, 2, 512};
    audio_spec obtained;
    
    auto handle = backend->open_device("", desired, obtained);
    REQUIRE(handle != 0);
    
    // Create test stream
    std::atomic<int> callback_count{0};
    auto stream = backend->create_stream(handle, obtained,
        [](void* data, uint8_t*, int) {
            (*static_cast<std::atomic<int>*>(data))++;
        },
        &callback_count
    );
    
    // Wait for callbacks
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(callback_count > 0);
}
```

### Integration Tests

```cpp
TEST_CASE("Backend with audio system") {
    // Register backend
    register_audio_backend("my_backend", 
        []() { return std::make_unique<my_backend>(); });
    
    // Use with audio system
    audio_device device;
    device.set_backend("my_backend");
    device.open_default_device();
    
    // Play test audio
    auto source = load_wav("test.wav");
    auto stream = device.create_stream(std::move(source));
    stream->play();
    
    // Verify playback
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(stream->get_state() == stream_state::playing);
}
```

## Common Patterns

### Device Handle Management

```cpp
class backend_with_handles : public audio_backend {
private:
    // Thread-safe handle generation
    std::atomic<uint32_t> m_next_handle{1};
    std::map<uint32_t, device_context> m_devices;
    mutable std::shared_mutex m_devices_mutex;
    
    device_context* get_device(uint32_t handle) {
        std::shared_lock lock(m_devices_mutex);
        auto it = m_devices.find(handle);
        return it != m_devices.end() ? &it->second : nullptr;
    }
    
public:
    uint32_t open_device(...) override {
        // ... open device ...
        
        std::unique_lock lock(m_devices_mutex);
        uint32_t handle = m_next_handle.fetch_add(1);
        m_devices[handle] = context;
        return handle;
    }
    
    void close_device(uint32_t handle) override {
        std::unique_lock lock(m_devices_mutex);
        m_devices.erase(handle);
    }
};
```

### Format Conversion

```cpp
// Convert musac format to platform format
platform_format convert_format(audio_format format) {
    switch (format) {
        case audio_format::u8:  return PLATFORM_U8;
        case audio_format::s16: return PLATFORM_S16;
        case audio_format::s32: return PLATFORM_S32;
        case audio_format::f32: return PLATFORM_F32;
        default:
            throw format_error("Unsupported format");
    }
}

// Handle endianness
platform_format get_platform_format(audio_format format) {
    if (is_big_endian()) {
        switch (format) {
            case audio_format::s16le: return PLATFORM_S16_SWAP;
            case audio_format::s32le: return PLATFORM_S32_SWAP;
            // ...
        }
    }
    return convert_format(format);
}
```

### Callback Dispatching

```cpp
class callback_backend : public audio_backend {
private:
    struct stream_context {
        void (*user_callback)(void*, uint8_t*, int);
        void* user_data;
        audio_spec spec;
        std::vector<uint8_t> conversion_buffer;
    };
    
    static void platform_callback(void* userdata, 
                                 void* stream,
                                 int len) {
        auto* ctx = static_cast<stream_context*>(userdata);
        
        // Convert format if needed
        if (needs_conversion(ctx->spec)) {
            // Use conversion buffer
            ctx->user_callback(ctx->user_data, 
                             ctx->conversion_buffer.data(),
                             len);
            convert_samples(ctx->conversion_buffer.data(),
                          stream, len, ctx->spec);
        } else {
            // Direct callback
            ctx->user_callback(ctx->user_data,
                             static_cast<uint8_t*>(stream),
                             len);
        }
    }
};
```

### Error Recovery

```cpp
uint32_t robust_backend::open_device(...) {
    const int max_retries = 3;
    std::exception_ptr last_error;
    
    for (int i = 0; i < max_retries; ++i) {
        try {
            return do_open_device(...);
        } catch (const std::exception& e) {
            last_error = std::current_exception();
            
            // Wait before retry
            std::this_thread::sleep_for(
                std::chrono::milliseconds(100 * (i + 1))
            );
            
            // Try to recover
            if (i < max_retries - 1) {
                try_recover();
            }
        }
    }
    
    std::rethrow_exception(last_error);
}
```

## Platform Integration

### Windows (WASAPI)

```cpp
#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

class wasapi_backend : public audio_backend {
private:
    ComPtr<IMMDeviceEnumerator> m_enumerator;
    
public:
    void init() override {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        
        HRESULT hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            &m_enumerator
        );
        
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create device enumerator");
        }
    }
    
    // ... WASAPI implementation
};
#endif
```

### macOS (CoreAudio)

```cpp
#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>

class coreaudio_backend : public audio_backend {
private:
    AudioComponent m_component;
    
public:
    void init() override {
        AudioComponentDescription desc = {
            .componentType = kAudioUnitType_Output,
            .componentSubType = kAudioUnitSubType_DefaultOutput,
            .componentManufacturer = kAudioUnitManufacturer_Apple
        };
        
        m_component = AudioComponentFindNext(nullptr, &desc);
        if (!m_component) {
            throw std::runtime_error("Failed to find audio component");
        }
    }
    
    // ... CoreAudio implementation
};
#endif
```

### Linux (ALSA)

```cpp
#ifdef __linux__
#include <alsa/asoundlib.h>

class alsa_backend : public audio_backend {
public:
    void init() override {
        // ALSA doesn't require global init
        m_initialized = true;
    }
    
    uint32_t open_device(const std::string& device_id,
                        const audio_spec& spec,
                        audio_spec& obtained) override {
        snd_pcm_t* handle;
        const char* device = device_id.empty() ? 
                            "default" : device_id.c_str();
        
        int err = snd_pcm_open(&handle, device,
                              SND_PCM_STREAM_PLAYBACK, 0);
        if (err < 0) {
            throw device_error(snd_strerror(err));
        }
        
        // Configure hardware parameters...
        
        return store_device(handle, obtained);
    }
    
    // ... ALSA implementation
};
#endif
```

## Best Practices

### Thread Safety

1. **Protect shared state**: Use mutexes for device maps
2. **Atomic operations**: Use atomics for flags and counters
3. **Lock hierarchy**: Define clear lock ordering to prevent deadlocks
4. **Reader/writer locks**: Use shared_mutex for read-heavy operations

### Resource Management

1. **RAII**: Use smart pointers and RAII wrappers
2. **Exception safety**: Ensure cleanup in destructors
3. **Handle validation**: Always validate device handles
4. **Leak prevention**: Track all allocated resources

### Performance

1. **Lock-free audio thread**: Never block in callbacks
2. **Buffer pooling**: Reuse buffers to avoid allocation
3. **Format caching**: Cache format conversions
4. **Batch operations**: Process audio in optimal chunk sizes

### Error Handling

1. **Descriptive errors**: Include context in error messages
2. **Recovery strategies**: Implement retry logic where appropriate
3. **Graceful degradation**: Fall back to simpler formats if needed
4. **Logging**: Add debug logging for troubleshooting

### Testing

1. **Mock devices**: Create mock devices for testing
2. **Stress testing**: Test with multiple simultaneous streams
3. **Format coverage**: Test all supported formats
4. **Error injection**: Test error handling paths
5. **Performance benchmarks**: Measure latency and CPU usage

## Conclusion

Creating a custom backend for musac allows you to integrate with any audio API or platform. Follow this guide's patterns and best practices to create robust, performant backends that integrate seamlessly with the musac audio system.

For additional examples, see the existing backend implementations:
- `src/backends/sdl3/` - SDL3 backend (cross-platform)
- `src/backends/sdl2/` - SDL2 backend (legacy)