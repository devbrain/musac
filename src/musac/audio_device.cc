#include <musac/audio_device.hh>
#include <musac/audio_stream_interface.hh>
#include <musac/sdk/audio_backend_v2.hh>
#include <musac/backends/sdl3/sdl3_backend.hh>
#include <musac/backends/null/null_backend.hh>
#include <musac/stream.hh>
#include <musac/audio_device_data.hh>
#include <musac/sdk/from_float_converter.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>

namespace musac {

// Forward declarations
extern void close_audio_stream();
extern void reset_audio_stream();

// Global backend instance for backward compatibility
static std::shared_ptr<audio_backend_v2> s_global_backend;
static std::mutex s_backend_mutex;


// Track the active device to enforce single device constraint
static audio_device* s_active_device = nullptr;
static std::mutex s_device_mutex;

// Helper function for device switching
audio_device* get_active_audio_device() {
    std::lock_guard<std::mutex> lock(s_device_mutex);
    return s_active_device;
}


// Helper functions to convert between v1 and v2 device info
static device_info to_v1_info(const device_info_v2& v2) {
    return {v2.name, v2.id, v2.is_default, v2.channels, v2.sample_rate};
}

static device_info_v2 to_v2_info(const device_info& v1) {
    return {v1.name, v1.id, v1.is_default, v1.channels, v1.sample_rate};
}

// Close all audio devices (called from audio_system::done)
void close_audio_devices() {
    {
        std::lock_guard<std::mutex> lock(s_device_mutex);
        s_active_device = nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(s_backend_mutex);
        if (s_global_backend && s_global_backend->is_initialized()) {
            s_global_backend->shutdown();
        }
        s_global_backend.reset();
    }
}

// New v2 API factory methods
std::vector<device_info> audio_device::enumerate_devices(
    std::shared_ptr<audio_backend_v2> backend,
    bool playback_devices) {
    
    if (!backend) {
        THROW_RUNTIME("Backend is null");
    }
    if (!backend->is_initialized()) {
        THROW_RUNTIME("Backend is not initialized");
    }
    
    auto devices_v2 = backend->enumerate_devices(playback_devices);
    std::vector<device_info> devices_v1;
    devices_v1.reserve(devices_v2.size());
    
    for (const auto& dev : devices_v2) {
        devices_v1.push_back(to_v1_info(dev));
    }
    
    return devices_v1;
}

audio_device audio_device::open_default_device(
    std::shared_ptr<audio_backend_v2> backend,
    const audio_spec* spec) {
    
    if (!backend) {
        THROW_RUNTIME("Backend is null");
    }
    if (!backend->is_initialized()) {
        THROW_RUNTIME("Backend is not initialized");
    }
    
    device_info_v2 info = backend->get_default_device(true);
    return audio_device(backend, info, spec);
}

audio_device audio_device::open_device(
    std::shared_ptr<audio_backend_v2> backend,
    const std::string& device_id, 
    const audio_spec* spec) {
    
    if (!backend) {
        THROW_RUNTIME("Backend is null");
    }
    if (!backend->is_initialized()) {
        THROW_RUNTIME("Backend is not initialized");
    }
    
    // Find the device with matching ID
    auto devices = backend->enumerate_devices(true);
    for (const auto& dev : devices) {
        if (dev.id == device_id) {
            return audio_device(backend, dev, spec);
        }
    }
    
    THROW_RUNTIME("Device not found: " + device_id);
}


// audio_device implementation
struct audio_device::impl {
    // Order matters! Members are destroyed in reverse order of declaration
    // 1. stream is destroyed first (may use device)
    std::unique_ptr<audio_stream_interface> stream;
    
    // 2. For v2 backend path
    std::shared_ptr<audio_backend_v2> backend_v2;
    uint32_t device_handle_v2 = 0;
    
    // 3. Common members
    audio_spec spec;
    std::atomic<int> stream_count{0}; // Track active streams
    std::string device_name;
    std::string device_id;
    
    // Destructor to handle cleanup
    ~impl() {
        if (backend_v2 && device_handle_v2) {
            backend_v2->close_device(device_handle_v2);
        }
    }
};


audio_device::~audio_device() {
    {
        std::lock_guard<std::mutex> lock(s_device_mutex);
        if (s_active_device == this) {
            // LOG_INFO("AudioDevice", "Destroying active device");
            s_active_device = nullptr;
        }
    }
    
    // RAII cleanup: The impl destructor will handle everything
}

// New v2 backend constructor
audio_device::audio_device(
    std::shared_ptr<audio_backend_v2> backend,
    const device_info_v2& info, 
    const audio_spec* desired_spec)
    : m_pimpl(std::make_unique<impl>()) {
    
    if (!backend) {
        THROW_RUNTIME("Backend is null");
    }
    if (!backend->is_initialized()) {
        THROW_RUNTIME("Backend is not initialized");
    }
    
    // Store backend reference
    m_pimpl->backend_v2 = backend;
    m_pimpl->device_name = info.name;
    m_pimpl->device_id = info.id;
    
    // Prepare audio spec
    audio_spec spec;
    if (desired_spec) {
        spec = *desired_spec;
    } else {
        // Use device defaults
        spec.format = audio_format::f32le;
        spec.channels = info.channels;
        spec.freq = info.sample_rate;
    }
    
    // Open device through v2 backend
    audio_spec obtained_spec;
    m_pimpl->device_handle_v2 = backend->open_device(
        info.id, spec, obtained_spec
    );
    
    if (!m_pimpl->device_handle_v2) {
        THROW_RUNTIME("Failed to open audio device: " + info.name);
    }
    
    m_pimpl->spec = obtained_spec;
    
    // Register as active device if there isn't one
    {
        std::lock_guard<std::mutex> lock(s_device_mutex);
        if (s_active_device == nullptr) {
            s_active_device = this;
            reset_audio_stream();
        }
    }
}

audio_device::audio_device(audio_device&&) noexcept = default;

std::string audio_device::get_device_name() const {
    return m_pimpl->device_name.empty() ? "Default Device" : m_pimpl->device_name;
}

std::string audio_device::get_device_id() const {
    return m_pimpl->device_id;
}

audio_format audio_device::get_format() const {
    return m_pimpl->spec.format;
}

channels_t audio_device::get_channels() const {
    return m_pimpl->spec.channels;
}

sample_rate_t audio_device::get_freq() const {
    return m_pimpl->spec.freq;
}

bool audio_device::pause() {
    if (m_pimpl->backend_v2 && m_pimpl->device_handle_v2) {
        return m_pimpl->backend_v2->pause_device(m_pimpl->device_handle_v2);
    }
    return false;
}

bool audio_device::is_paused() const {
    if (m_pimpl->backend_v2 && m_pimpl->device_handle_v2) {
        return m_pimpl->backend_v2->is_device_paused(m_pimpl->device_handle_v2);
    }
    return false;
}

bool audio_device::resume() {
    if (m_pimpl->backend_v2 && m_pimpl->device_handle_v2) {
        return m_pimpl->backend_v2->resume_device(m_pimpl->device_handle_v2);
    }
    return false;
}

float audio_device::get_gain() const {
    if (m_pimpl->backend_v2 && m_pimpl->device_handle_v2) {
        return m_pimpl->backend_v2->get_device_gain(m_pimpl->device_handle_v2);
    }
    return 0.0f;
}

void audio_device::set_gain(float v) {
    if (m_pimpl->backend_v2 && m_pimpl->device_handle_v2) {
        m_pimpl->backend_v2->set_device_gain(m_pimpl->device_handle_v2, v);
    }
}

// Create stream with callback
void audio_device::create_stream_with_callback(
    void (*callback)(void* userdata, uint8_t* stream, int len),
    void* userdata) {
    
    if (m_pimpl->backend_v2 && m_pimpl->device_handle_v2) {
        m_pimpl->stream = m_pimpl->backend_v2->create_stream(
            m_pimpl->device_handle_v2, m_pimpl->spec, callback, userdata
        );
        return;
    }
    
    THROW_RUNTIME("No device available");
}

audio_stream audio_device::create_stream(audio_source&& audio_src) {
    // Set up audio device data for the stream system
    audio_device_data aud;
    aud.m_audio_spec = m_pimpl->spec;
    // m_stream is no longer used - we use direct conversion instead of SDL streams
    aud.m_frame_size = 4096; // Default frame size
    aud.m_sample_converter = get_from_float_converter(m_pimpl->spec.format);
    
    // Pre-calculate bytes per sample for performance
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
            LOG_ERROR("audio_device", "Unknown audio format");
            aud.m_bytes_per_sample = 2; // Default to 16-bit
            break;
    }
    aud.m_bytes_per_frame = aud.m_bytes_per_sample * m_pimpl->spec.channels;
    aud.m_ms_per_frame = 1000.0f / static_cast<float>(m_pimpl->spec.freq);
    
    audio_stream::set_audio_device_data(aud);
    
    // Create the callback that will call audio_stream's callback
    if (!m_pimpl->stream) {
        create_stream_with_callback(
            [](void* userdata, uint8_t* stream, int len) {
                audio_stream::audio_callback(stream, static_cast<unsigned int>(len));
            },
            nullptr
        );
    }
    
    return audio_stream(std::move(audio_src));
}

} // namespace musac