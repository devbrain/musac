# Device Management Guide

This guide covers all aspects of audio device management in Musac, including device enumeration, selection, hot-plug handling, and device properties.

## Table of Contents

1. [Device Enumeration](#device-enumeration)
2. [Device Selection](#device-selection)
3. [Hot-Plug Handling](#hot-plug-handling)
4. [Device Properties](#device-properties)
5. [Device Switching](#device-switching)
6. [Error Recovery](#error-recovery)
7. [Platform-Specific Considerations](#platform-specific-considerations)
8. [Best Practices](#best-practices)

## Device Enumeration

### Listing Available Devices

```cpp
#include <musac/audio_device.hh>
#include <iostream>

void list_audio_devices() {
    musac::audio_device device;
    
    // Get all available audio devices
    auto devices = device.enumerate_devices();
    
    std::cout << "Available audio devices:\n";
    for (const auto& info : devices) {
        std::cout << "  ID: " << info.id << "\n";
        std::cout << "    Name: " << info.name << "\n";
        std::cout << "    Type: " << (info.is_capture ? "Capture" : "Playback") << "\n";
        std::cout << "    Channels: " << info.channels << "\n";
        std::cout << "    Sample Rate: " << info.sample_rate << " Hz\n";
        std::cout << "    Is Default: " << (info.is_default ? "Yes" : "No") << "\n";
        std::cout << "\n";
    }
}
```

### Device Information Structure

```cpp
struct AudioDeviceInfo {
    std::string id;           // Unique device identifier
    std::string name;          // Human-readable name
    int channels;              // Number of channels (2 for stereo)
    int sample_rate;           // Native sample rate
    bool is_capture;           // Input device flag
    bool is_default;           // System default device
    
    // Additional properties
    int buffer_size_min;       // Minimum buffer size
    int buffer_size_max;       // Maximum buffer size
    std::vector<int> supported_rates;  // Supported sample rates
};
```

### Filtering Devices

```cpp
class DeviceFilter {
public:
    static std::vector<musac::AudioDeviceInfo> 
    get_output_devices(const std::vector<musac::AudioDeviceInfo>& all_devices) {
        std::vector<musac::AudioDeviceInfo> output_devices;
        
        std::copy_if(all_devices.begin(), all_devices.end(),
                    std::back_inserter(output_devices),
                    [](const auto& info) { return !info.is_capture; });
        
        return output_devices;
    }
    
    static std::vector<musac::AudioDeviceInfo> 
    get_stereo_devices(const std::vector<musac::AudioDeviceInfo>& all_devices) {
        std::vector<musac::AudioDeviceInfo> stereo_devices;
        
        std::copy_if(all_devices.begin(), all_devices.end(),
                    std::back_inserter(stereo_devices),
                    [](const auto& info) { return info.channels >= 2; });
        
        return stereo_devices;
    }
    
    static std::optional<musac::AudioDeviceInfo> 
    find_device_by_name(const std::vector<musac::AudioDeviceInfo>& devices,
                       const std::string& name_pattern) {
        auto it = std::find_if(devices.begin(), devices.end(),
            [&name_pattern](const auto& info) {
                return info.name.find(name_pattern) != std::string::npos;
            });
        
        if (it != devices.end()) {
            return *it;
        }
        return std::nullopt;
    }
};
```

## Device Selection

### Opening Default Device

```cpp
musac::audio_device device;

// Open system default output device
device.open_default_device();

// Check if device opened successfully
if (!device.is_open()) {
    throw std::runtime_error("Failed to open default audio device");
}
```

### Opening Specific Device

```cpp
class DeviceSelector {
private:
    musac::audio_device m_device;
    
public:
    bool open_device_by_id(const std::string& device_id) {
        try {
            m_device.open_device(device_id);
            return true;
        } catch (const musac::device_error& e) {
            std::cerr << "Failed to open device: " << e.what() << "\n";
            return false;
        }
    }
    
    bool open_device_by_name(const std::string& name_pattern) {
        auto devices = m_device.enumerate_devices();
        
        auto device_info = DeviceFilter::find_device_by_name(devices, name_pattern);
        if (device_info) {
            return open_device_by_id(device_info->id);
        }
        
        std::cerr << "No device found matching: " << name_pattern << "\n";
        return false;
    }
    
    bool open_preferred_device(const std::vector<std::string>& preferences) {
        // Try devices in order of preference
        for (const auto& pref : preferences) {
            if (open_device_by_name(pref)) {
                std::cout << "Opened device: " << pref << "\n";
                return true;
            }
        }
        
        // Fall back to default
        std::cout << "Opening default device\n";
        m_device.open_default_device();
        return m_device.is_open();
    }
};
```

### Device Configuration

```cpp
class DeviceConfigurator {
public:
    struct DeviceConfig {
        int sample_rate = 44100;
        int channels = 2;
        int buffer_size = 1024;
        bool exclusive_mode = false;
    };
    
    static void configure_device(musac::audio_device& device,
                                const DeviceConfig& config) {
        // Set sample rate
        device.set_sample_rate(config.sample_rate);
        
        // Set channel count
        device.set_channels(config.channels);
        
        // Set buffer size
        device.set_buffer_size(config.buffer_size);
        
        // Set exclusive mode (if supported)
        if (config.exclusive_mode) {
            device.set_exclusive_mode(true);
        }
    }
    
    static DeviceConfig get_optimal_config(const musac::AudioDeviceInfo& info) {
        DeviceConfig config;
        
        // Use device's native sample rate for best performance
        config.sample_rate = info.sample_rate;
        config.channels = std::min(info.channels, 2);  // Stereo max
        
        // Choose buffer size based on use case
        if (info.buffer_size_min > 0 && info.buffer_size_max > 0) {
            // Low latency: use smaller buffer
            // config.buffer_size = info.buffer_size_min * 2;
            
            // Balanced: use medium buffer
            config.buffer_size = (info.buffer_size_min + info.buffer_size_max) / 2;
            
            // Stability: use larger buffer
            // config.buffer_size = info.buffer_size_max / 2;
        }
        
        return config;
    }
};
```

## Hot-Plug Handling

### Device Change Detection

```cpp
class DeviceHotPlugHandler {
private:
    musac::audio_device* m_device;
    std::function<void()> m_on_device_added;
    std::function<void()> m_on_device_removed;
    std::function<void(const std::string&)> m_on_default_changed;
    std::thread m_monitor_thread;
    std::atomic<bool> m_monitoring{false};
    
public:
    void start_monitoring(musac::audio_device* device) {
        m_device = device;
        m_monitoring.store(true);
        
        // Register callbacks with the audio system
        device->set_device_change_callback(
            [this](musac::DeviceChangeEvent event) {
                handle_device_change(event);
            }
        );
        
        // Start monitoring thread for polling-based systems
        m_monitor_thread = std::thread([this]() {
            monitor_devices();
        });
    }
    
    void stop_monitoring() {
        m_monitoring.store(false);
        if (m_monitor_thread.joinable()) {
            m_monitor_thread.join();
        }
    }
    
    void on_device_added(std::function<void()> callback) {
        m_on_device_added = callback;
    }
    
    void on_device_removed(std::function<void()> callback) {
        m_on_device_removed = callback;
    }
    
    void on_default_changed(std::function<void(const std::string&)> callback) {
        m_on_default_changed = callback;
    }
    
private:
    void handle_device_change(musac::DeviceChangeEvent event) {
        switch (event.type) {
            case musac::DeviceChangeEvent::ADDED:
                std::cout << "Device added: " << event.device_name << "\n";
                if (m_on_device_added) m_on_device_added();
                break;
                
            case musac::DeviceChangeEvent::REMOVED:
                std::cout << "Device removed: " << event.device_name << "\n";
                if (m_on_device_removed) m_on_device_removed();
                handle_device_removal();
                break;
                
            case musac::DeviceChangeEvent::DEFAULT_CHANGED:
                std::cout << "Default device changed to: " << event.device_name << "\n";
                if (m_on_default_changed) m_on_default_changed(event.device_name);
                break;
        }
    }
    
    void handle_device_removal() {
        // Check if current device was removed
        if (!m_device->is_device_valid()) {
            std::cout << "Current device lost, switching to default\n";
            
            // Attempt to switch to default device
            try {
                m_device->open_default_device();
            } catch (const musac::device_error& e) {
                std::cerr << "Failed to switch to default device: " << e.what() << "\n";
            }
        }
    }
    
    void monitor_devices() {
        std::vector<musac::AudioDeviceInfo> previous_devices;
        
        while (m_monitoring.load()) {
            auto current_devices = m_device->enumerate_devices();
            
            if (current_devices != previous_devices) {
                detect_changes(previous_devices, current_devices);
                previous_devices = current_devices;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    void detect_changes(const std::vector<musac::AudioDeviceInfo>& old_list,
                       const std::vector<musac::AudioDeviceInfo>& new_list) {
        // Find added devices
        for (const auto& new_device : new_list) {
            auto it = std::find_if(old_list.begin(), old_list.end(),
                [&new_device](const auto& old) {
                    return old.id == new_device.id;
                });
            
            if (it == old_list.end()) {
                // Device was added
                musac::DeviceChangeEvent event;
                event.type = musac::DeviceChangeEvent::ADDED;
                event.device_name = new_device.name;
                handle_device_change(event);
            }
        }
        
        // Find removed devices
        for (const auto& old_device : old_list) {
            auto it = std::find_if(new_list.begin(), new_list.end(),
                [&old_device](const auto& new_dev) {
                    return new_dev.id == old_device.id;
                });
            
            if (it == new_list.end()) {
                // Device was removed
                musac::DeviceChangeEvent event;
                event.type = musac::DeviceChangeEvent::REMOVED;
                event.device_name = old_device.name;
                handle_device_change(event);
            }
        }
    }
};
```

### Automatic Device Recovery

```cpp
class AutoRecoveryManager {
private:
    musac::audio_device* m_device;
    std::vector<std::unique_ptr<musac::audio_stream>> m_streams;
    bool m_auto_recover = true;
    
public:
    void enable_auto_recovery(musac::audio_device* device) {
        m_device = device;
        
        // Set up device loss handler
        device->set_device_lost_callback([this]() {
            if (m_auto_recover) {
                attempt_recovery();
            }
        });
    }
    
    void register_stream(std::unique_ptr<musac::audio_stream> stream) {
        m_streams.push_back(std::move(stream));
    }
    
private:
    void attempt_recovery() {
        std::cout << "Attempting device recovery...\n";
        
        // Save stream states
        struct StreamState {
            bool was_playing;
            float volume;
            float pan;
            std::chrono::microseconds position;
        };
        
        std::vector<StreamState> states;
        for (const auto& stream : m_streams) {
            states.push_back({
                stream->is_playing(),
                stream->get_volume(),
                stream->get_stereo_pos(),
                stream->tell_time()
            });
        }
        
        // Try to reopen device
        int attempts = 0;
        const int max_attempts = 5;
        
        while (attempts < max_attempts) {
            try {
                m_device->open_default_device();
                std::cout << "Device recovered successfully\n";
                
                // Restore stream states
                restore_streams(states);
                return;
                
            } catch (const musac::device_error& e) {
                attempts++;
                std::cout << "Recovery attempt " << attempts << " failed: " 
                         << e.what() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
        std::cerr << "Failed to recover audio device after " 
                  << max_attempts << " attempts\n";
    }
    
    void restore_streams(const std::vector<StreamState>& states) {
        for (size_t i = 0; i < m_streams.size() && i < states.size(); ++i) {
            auto& stream = m_streams[i];
            const auto& state = states[i];
            
            // Restore volume and pan
            stream->set_volume(state.volume);
            stream->set_stereo_pos(state.pan);
            
            // Restore playback position
            stream->seek_to_time(state.position);
            
            // Resume playback if needed
            if (state.was_playing) {
                stream->play();
            }
        }
    }
};
```

## Device Properties

### Querying Device Capabilities

```cpp
class DeviceCapabilities {
public:
    struct Capabilities {
        std::vector<int> supported_sample_rates;
        std::vector<int> supported_channel_counts;
        int min_buffer_size;
        int max_buffer_size;
        bool supports_exclusive_mode;
        bool supports_low_latency;
        float min_latency_ms;
        float max_latency_ms;
    };
    
    static Capabilities query_device(musac::audio_device& device) {
        Capabilities caps;
        
        // Query supported sample rates
        caps.supported_sample_rates = device.get_supported_sample_rates();
        
        // Query channel support
        caps.supported_channel_counts = device.get_supported_channel_counts();
        
        // Query buffer limits
        caps.min_buffer_size = device.get_min_buffer_size();
        caps.max_buffer_size = device.get_max_buffer_size();
        
        // Query feature support
        caps.supports_exclusive_mode = device.supports_exclusive_mode();
        caps.supports_low_latency = device.supports_low_latency();
        
        // Calculate latency
        int current_rate = device.get_sample_rate();
        caps.min_latency_ms = (caps.min_buffer_size * 1000.0f) / current_rate;
        caps.max_latency_ms = (caps.max_buffer_size * 1000.0f) / current_rate;
        
        return caps;
    }
    
    static void print_capabilities(const Capabilities& caps) {
        std::cout << "Device Capabilities:\n";
        
        std::cout << "  Sample rates: ";
        for (int rate : caps.supported_sample_rates) {
            std::cout << rate << " ";
        }
        std::cout << "Hz\n";
        
        std::cout << "  Channel counts: ";
        for (int ch : caps.supported_channel_counts) {
            std::cout << ch << " ";
        }
        std::cout << "\n";
        
        std::cout << "  Buffer size: " << caps.min_buffer_size 
                  << " - " << caps.max_buffer_size << " samples\n";
        
        std::cout << "  Latency: " << caps.min_latency_ms 
                  << " - " << caps.max_latency_ms << " ms\n";
        
        std::cout << "  Exclusive mode: " 
                  << (caps.supports_exclusive_mode ? "Yes" : "No") << "\n";
        
        std::cout << "  Low latency: " 
                  << (caps.supports_low_latency ? "Yes" : "No") << "\n";
    }
};
```

### Device Performance Metrics

```cpp
class DeviceMetrics {
private:
    struct Metrics {
        std::atomic<uint64_t> samples_processed{0};
        std::atomic<uint64_t> underruns{0};
        std::atomic<uint64_t> overruns{0};
        std::atomic<float> cpu_usage{0.0f};
        std::chrono::steady_clock::time_point start_time;
    };
    
    Metrics m_metrics;
    
public:
    void start_monitoring() {
        m_metrics.start_time = std::chrono::steady_clock::now();
        m_metrics.samples_processed = 0;
        m_metrics.underruns = 0;
        m_metrics.overruns = 0;
    }
    
    void update_samples(uint64_t samples) {
        m_metrics.samples_processed += samples;
    }
    
    void record_underrun() {
        m_metrics.underruns++;
    }
    
    void record_overrun() {
        m_metrics.overruns++;
    }
    
    void update_cpu_usage(float usage) {
        m_metrics.cpu_usage.store(usage);
    }
    
    void print_metrics() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>
                       (now - m_metrics.start_time).count();
        
        if (duration == 0) duration = 1;  // Avoid division by zero
        
        std::cout << "Device Performance Metrics:\n";
        std::cout << "  Runtime: " << duration << " seconds\n";
        std::cout << "  Samples processed: " << m_metrics.samples_processed << "\n";
        std::cout << "  Average sample rate: " 
                  << (m_metrics.samples_processed / duration) << " Hz\n";
        std::cout << "  Underruns: " << m_metrics.underruns << "\n";
        std::cout << "  Overruns: " << m_metrics.overruns << "\n";
        std::cout << "  CPU usage: " << m_metrics.cpu_usage.load() << "%\n";
        
        float underrun_rate = static_cast<float>(m_metrics.underruns) / duration;
        if (underrun_rate > 0.1f) {
            std::cout << "  WARNING: High underrun rate (" 
                     << underrun_rate << " per second)\n";
        }
    }
};
```

## Device Switching

### Live Device Switching

```cpp
class DeviceSwitcher {
private:
    musac::audio_device* m_current_device;
    std::vector<musac::audio_stream*> m_active_streams;
    
public:
    bool switch_device(const std::string& new_device_id) {
        // Save current stream states
        std::vector<StreamSnapshot> snapshots;
        for (auto stream : m_active_streams) {
            snapshots.push_back(create_snapshot(stream));
        }
        
        // Create new device
        auto new_device = std::make_unique<musac::audio_device>();
        
        try {
            new_device->open_device(new_device_id);
        } catch (const musac::device_error& e) {
            std::cerr << "Failed to open new device: " << e.what() << "\n";
            return false;
        }
        
        // Stop all streams on old device
        for (auto stream : m_active_streams) {
            stream->stop();
        }
        
        // Switch devices
        musac::audio_device* old_device = m_current_device;
        m_current_device = new_device.release();
        
        // Recreate streams on new device
        recreate_streams(snapshots);
        
        // Clean up old device
        delete old_device;
        
        return true;
    }
    
private:
    struct StreamSnapshot {
        std::string source_path;
        float volume;
        float pan;
        bool was_playing;
        std::chrono::microseconds position;
        int loop_count;
    };
    
    StreamSnapshot create_snapshot(musac::audio_stream* stream) {
        return {
            stream->get_source_path(),
            stream->get_volume(),
            stream->get_stereo_pos(),
            stream->is_playing(),
            stream->tell_time(),
            stream->get_loop_count()
        };
    }
    
    void recreate_streams(const std::vector<StreamSnapshot>& snapshots) {
        m_active_streams.clear();
        
        for (const auto& snapshot : snapshots) {
            // Recreate stream
            auto source = musac::audio_loader::load(snapshot.source_path);
            auto stream = m_current_device->create_stream(std::move(source));
            
            // Restore state
            stream->set_volume(snapshot.volume);
            stream->set_stereo_pos(snapshot.pan);
            stream->set_loop_count(snapshot.loop_count);
            stream->seek_to_time(snapshot.position);
            
            if (snapshot.was_playing) {
                stream->play();
            }
            
            m_active_streams.push_back(stream.release());
        }
    }
};
```

## Error Recovery

### Robust Device Management

```cpp
class RobustDeviceManager {
private:
    musac::audio_device m_device;
    std::string m_preferred_device_id;
    int m_retry_count = 0;
    const int MAX_RETRIES = 3;
    
public:
    bool initialize() {
        // Try preferred device first
        if (!m_preferred_device_id.empty()) {
            if (try_open_device(m_preferred_device_id)) {
                return true;
            }
        }
        
        // Try default device
        if (try_open_default()) {
            return true;
        }
        
        // Try any available device
        return try_any_device();
    }
    
    void set_preferred_device(const std::string& device_id) {
        m_preferred_device_id = device_id;
    }
    
private:
    bool try_open_device(const std::string& device_id) {
        try {
            m_device.open_device(device_id);
            std::cout << "Opened device: " << device_id << "\n";
            m_retry_count = 0;
            return true;
        } catch (const musac::device_error& e) {
            std::cerr << "Failed to open device " << device_id 
                     << ": " << e.what() << "\n";
            return false;
        }
    }
    
    bool try_open_default() {
        try {
            m_device.open_default_device();
            std::cout << "Opened default device\n";
            m_retry_count = 0;
            return true;
        } catch (const musac::device_error& e) {
            std::cerr << "Failed to open default device: " << e.what() << "\n";
            return false;
        }
    }
    
    bool try_any_device() {
        auto devices = m_device.enumerate_devices();
        
        for (const auto& info : devices) {
            if (!info.is_capture && try_open_device(info.id)) {
                return true;
            }
        }
        
        std::cerr << "No available audio devices found\n";
        return false;
    }
    
public:
    void handle_device_error() {
        m_retry_count++;
        
        if (m_retry_count > MAX_RETRIES) {
            std::cerr << "Max retries exceeded, giving up\n";
            return;
        }
        
        std::cout << "Attempting to recover (attempt " 
                  << m_retry_count << "/" << MAX_RETRIES << ")\n";
        
        // Wait before retry
        std::this_thread::sleep_for(
            std::chrono::seconds(m_retry_count)  // Exponential backoff
        );
        
        initialize();
    }
};
```

## Platform-Specific Considerations

### Windows (WASAPI)

```cpp
#ifdef _WIN32
class WindowsDeviceManager {
public:
    static void configure_for_windows(musac::audio_device& device) {
        // Enable exclusive mode for lower latency
        device.set_exclusive_mode(true);
        
        // Use WASAPI event-driven mode
        device.set_callback_mode(musac::CallbackMode::EVENT_DRIVEN);
        
        // Set process priority
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        
        // Configure for multimedia
        configure_multimedia_timer();
    }
    
private:
    static void configure_multimedia_timer() {
        // Increase timer resolution for better audio timing
        timeBeginPeriod(1);
        
        // Register cleanup
        std::atexit([]() { timeEndPeriod(1); });
    }
};
#endif
```

### macOS (CoreAudio)

```cpp
#ifdef __APPLE__
class MacDeviceManager {
public:
    static void configure_for_mac(musac::audio_device& device) {
        // Use CoreAudio's default buffer size
        device.set_buffer_size(512);
        
        // Enable sample rate conversion if needed
        device.set_sample_rate_conversion(true);
        
        // Request audio workload scheduling
        request_audio_priority();
    }
    
private:
    static void request_audio_priority() {
        // Request real-time scheduling for audio thread
        // Implementation depends on macOS version
    }
};
#endif
```

### Linux (ALSA/PulseAudio)

```cpp
#ifdef __linux__
class LinuxDeviceManager {
public:
    static void configure_for_linux(musac::audio_device& device) {
        // Check if running under PulseAudio or ALSA
        if (is_pulseaudio_running()) {
            configure_pulseaudio(device);
        } else {
            configure_alsa(device);
        }
    }
    
private:
    static bool is_pulseaudio_running() {
        // Check if PulseAudio daemon is running
        return system("pulseaudio --check") == 0;
    }
    
    static void configure_pulseaudio(musac::audio_device& device) {
        // PulseAudio configuration
        device.set_buffer_size(1024);  // Good default for PA
        device.set_backend_specific_option("pulse.latency", "low");
    }
    
    static void configure_alsa(musac::audio_device& device) {
        // ALSA configuration
        device.set_buffer_size(256);  // Lower latency possible
        device.set_backend_specific_option("alsa.period_size", "64");
    }
};
#endif
```

## Best Practices

### 1. Device Selection Strategy

```cpp
class DeviceSelectionStrategy {
public:
    static std::string select_best_device() {
        auto devices = musac::audio_device::enumerate_devices();
        
        // Priority order:
        // 1. User's preferred device (from settings)
        // 2. System default device
        // 3. Device with best specs
        // 4. Any available device
        
        // Check for user preference
        std::string preferred = load_user_preference();
        if (!preferred.empty() && device_exists(devices, preferred)) {
            return preferred;
        }
        
        // Find default device
        auto default_device = find_default(devices);
        if (default_device) {
            return default_device->id;
        }
        
        // Find best specs
        auto best_device = find_best_specs(devices);
        if (best_device) {
            return best_device->id;
        }
        
        // Return any available
        if (!devices.empty()) {
            return devices[0].id;
        }
        
        throw std::runtime_error("No audio devices available");
    }
    
private:
    static std::optional<musac::AudioDeviceInfo> 
    find_default(const std::vector<musac::AudioDeviceInfo>& devices) {
        auto it = std::find_if(devices.begin(), devices.end(),
            [](const auto& d) { return d.is_default && !d.is_capture; });
        
        if (it != devices.end()) {
            return *it;
        }
        return std::nullopt;
    }
    
    static std::optional<musac::AudioDeviceInfo> 
    find_best_specs(const std::vector<musac::AudioDeviceInfo>& devices) {
        // Find device with best specs (highest sample rate, most channels)
        auto it = std::max_element(devices.begin(), devices.end(),
            [](const auto& a, const auto& b) {
                if (a.is_capture != b.is_capture) {
                    return a.is_capture;  // Prefer output devices
                }
                if (a.sample_rate != b.sample_rate) {
                    return a.sample_rate < b.sample_rate;
                }
                return a.channels < b.channels;
            });
        
        if (it != devices.end() && !it->is_capture) {
            return *it;
        }
        return std::nullopt;
    }
};
```

### 2. Error Handling

```cpp
void safe_device_operation() {
    try {
        musac::audio_device device;
        device.open_default_device();
        
        // Use device...
        
    } catch (const musac::device_error& e) {
        // Specific device error
        handle_device_error(e);
    } catch (const musac::backend_error& e) {
        // Backend-specific error
        handle_backend_error(e);
    } catch (const std::exception& e) {
        // General error
        handle_general_error(e);
    }
}
```

### 3. Resource Management

```cpp
class DeviceResourceManager {
private:
    std::unique_ptr<musac::audio_device> m_device;
    std::vector<std::unique_ptr<musac::audio_stream>> m_streams;
    
public:
    ~DeviceResourceManager() {
        // Ensure proper cleanup order
        m_streams.clear();  // Stop and destroy streams first
        m_device.reset();   // Then close device
    }
    
    void cleanup() {
        // Stop all streams
        for (auto& stream : m_streams) {
            if (stream->is_playing()) {
                stream->stop();
            }
        }
        
        // Wait for streams to finish
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Clear streams
        m_streams.clear();
        
        // Close device
        if (m_device && m_device->is_open()) {
            m_device->close();
        }
    }
};
```

## Troubleshooting

### Device Not Found

```cpp
void diagnose_device_issues() {
    std::cout << "Diagnosing audio device issues...\n";
    
    // List all devices
    auto devices = musac::audio_device::enumerate_devices();
    std::cout << "Found " << devices.size() << " audio devices\n";
    
    if (devices.empty()) {
        std::cout << "No audio devices found. Possible causes:\n";
        std::cout << "  - Audio service not running\n";
        std::cout << "  - No audio hardware installed\n";
        std::cout << "  - Driver issues\n";
        return;
    }
    
    // Check for output devices
    auto output_devices = std::count_if(devices.begin(), devices.end(),
        [](const auto& d) { return !d.is_capture; });
    
    std::cout << "Output devices: " << output_devices << "\n";
    
    if (output_devices == 0) {
        std::cout << "No output devices available\n";
        std::cout << "All devices are input/capture devices\n";
    }
}
```

### Performance Issues

```cpp
void optimize_device_performance(musac::audio_device& device) {
    std::cout << "Optimizing device performance...\n";
    
    // Get current settings
    int buffer_size = device.get_buffer_size();
    int sample_rate = device.get_sample_rate();
    
    std::cout << "Current: " << buffer_size << " samples @ " 
              << sample_rate << " Hz\n";
    
    // Calculate latency
    float latency_ms = (buffer_size * 1000.0f) / sample_rate;
    std::cout << "Latency: " << latency_ms << " ms\n";
    
    if (latency_ms > 20.0f) {
        std::cout << "High latency detected. Recommendations:\n";
        std::cout << "  - Reduce buffer size\n";
        std::cout << "  - Use exclusive mode if available\n";
        std::cout << "  - Close other audio applications\n";
    }
    
    // Check for xruns
    auto stats = device.get_statistics();
    if (stats.underruns > 0) {
        std::cout << "Underruns detected: " << stats.underruns << "\n";
        std::cout << "  - Increase buffer size\n";
        std::cout << "  - Reduce system load\n";
        std::cout << "  - Check process priority\n";
    }
}
```

## See Also

- [Playing Audio Files](PLAYING_AUDIO_FILES.md)
- [Backend Development Guide](BACKEND_DEVELOPMENT_GUIDE.md)
- [Error Handling Guide](ERROR_HANDLING_GUIDE.md)
- [API Reference](doxygen/html/index.html)