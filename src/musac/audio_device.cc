#include <musac/audio_device.hh>
#include <musac/audio_device_interface.hh>
#include <musac/audio_stream_interface.hh>
#include <musac/audio_backend.hh>
#include <musac/stream.hh>
#include <musac/audio_device_data.hh>
#include <musac/sdk/from_float_converter.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include "device_guard.hh"
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>

namespace musac {

// Forward declarations
extern void close_audio_stream();
extern void reset_audio_stream();

// Global device manager instance - use shared_ptr for safe lifetime management
static std::shared_ptr<audio_device_interface> s_device_manager;
static std::mutex s_manager_mutex;

// Track the active device to enforce single device constraint
static audio_device* s_active_device = nullptr;
static std::mutex s_device_mutex;

// Helper function for device switching
audio_device* get_active_audio_device() {
    std::lock_guard<std::mutex> lock(s_device_mutex);
    return s_active_device;
}

// Initialize device manager if needed
static std::shared_ptr<audio_device_interface> get_device_manager() {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    if (!s_device_manager) {
        // Need to include the factory header to get create_default_audio_device_manager
        extern std::unique_ptr<audio_device_interface> create_default_audio_device_manager();
        s_device_manager = create_default_audio_device_manager();
    }
    return s_device_manager;
}

// Close all audio devices (called from audio_system::done)
void close_audio_devices() {
    {
        std::lock_guard<std::mutex> lock(s_device_mutex);
        s_active_device = nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(s_manager_mutex);
        s_device_manager.reset();
    }
}

// Static factory methods
std::vector<device_info> audio_device::enumerate_devices(bool playback_devices) {
    auto manager = get_device_manager();
    if (!manager) {
        return {};
    }
    
    return manager->enumerate_devices(playback_devices);
}

audio_device audio_device::open_default_device(const audio_spec* spec) {
    auto manager = get_device_manager();
    if (!manager) {
        THROW_RUNTIME("No audio device manager available");
    }
    
    device_info info = manager->get_default_device(true);
    return audio_device(info, spec);
}

audio_device audio_device::open_device(const std::string& device_id, const audio_spec* spec) {
    auto manager = get_device_manager();
    if (!manager) {
        THROW_RUNTIME("No audio device manager available");
    }
    
    // Find the device with matching ID
    auto devices = manager->enumerate_devices(true);
    for (const auto& dev : devices) {
        if (dev.id == device_id) {
            return audio_device(dev, spec);
        }
    }
    
    THROW_RUNTIME("Device not found: " + device_id);
    // This is unreachable due to THROW_RUNTIME, but needed to satisfy compiler
    return audio_device(devices[0], spec);
}

// audio_device implementation
struct audio_device::impl {
    // Order matters! Members are destroyed in reverse order of declaration
    // 1. stream is destroyed first (may use device)
    std::unique_ptr<audio_stream_interface> stream;
    
    // 2. device_guard is destroyed second (closes device after stream is gone)
    device_guard device;
    
    // 3. Other members don't have cleanup dependencies
    audio_spec spec;
    std::atomic<int> stream_count{0}; // Track active streams
};

audio_device::audio_device(const device_info& info, const audio_spec* desired_spec)
    : m_pimpl(std::make_unique<impl>()) {
    
    // No longer enforce single device constraint since we support device switching
    
    auto manager = get_device_manager();
    if (!manager) {
        THROW_RUNTIME("No audio device manager available");
    }
    
    // Use device info to open device
    audio_spec spec;
    if (desired_spec) {
        spec = *desired_spec;
    } else {
        // Use device defaults - we need to get format from the device manager
        // For now, use sensible defaults
        spec.format = audio_format::f32le;
        spec.channels = info.channels;
        spec.freq = info.sample_rate;
    }
    
    audio_spec obtained_spec;
    uint32_t handle = manager->open_device(
        info.id, spec, obtained_spec
    );
    
    if (!handle) {
        THROW_RUNTIME("Failed to open audio device: " + info.name);
    }
    
    m_pimpl->spec = obtained_spec;
    
    // Initialize device guard - will auto-close on destruction
    m_pimpl->device = device_guard(manager, handle);
    
    // Only register as active device if there isn't one already
    // Otherwise, user must explicitly switch using audio_system::switch_device
    {
        std::lock_guard<std::mutex> lock(s_device_mutex);
        if (s_active_device == nullptr) {
            s_active_device = this;
            // Reset the audio stream state for the new device
            reset_audio_stream();
        }
    }
}

audio_device::~audio_device() {
    {
        std::lock_guard<std::mutex> lock(s_device_mutex);
        if (s_active_device == this) {
            // LOG_INFO("AudioDevice", "Destroying active device");
            s_active_device = nullptr;
        }
    }
    
    // RAII cleanup: The impl destructor will handle everything in the correct order:
    // 1. m_pimpl->stream will be destroyed first (may still use device)
    // 2. m_pimpl->device (device_guard) will be destroyed second (closes device)
    // No manual cleanup needed thanks to RAII!
}

audio_device::audio_device(audio_device&&) noexcept = default;

std::string audio_device::get_device_name() const {
    // We would need to store the device name in impl
    return "Default Device";
}

std::string audio_device::get_device_id() const {
    // We would need to store the device id in impl
    return "";
}

audio_format audio_device::get_format() const {
    return m_pimpl->spec.format;
}

int audio_device::get_channels() const {
    return m_pimpl->spec.channels;
}

int audio_device::get_freq() const {
    return m_pimpl->spec.freq;
}

bool audio_device::pause() {
    if (m_pimpl->device.valid()) {
        return m_pimpl->device.manager()->pause_device(m_pimpl->device.handle());
    }
    return false;
}

bool audio_device::is_paused() const {
    if (m_pimpl->device.valid()) {
        return m_pimpl->device.manager()->is_device_paused(m_pimpl->device.handle());
    }
    return false;
}

bool audio_device::resume() {
    if (m_pimpl->device.valid()) {
        return m_pimpl->device.manager()->resume_device(m_pimpl->device.handle());
    }
    return false;
}

float audio_device::get_gain() const {
    if (m_pimpl->device.valid()) {
        return m_pimpl->device.manager()->get_device_gain(m_pimpl->device.handle());
    }
    return 0.0f;
}

void audio_device::set_gain(float v) {
    if (m_pimpl->device.valid()) {
        m_pimpl->device.manager()->set_device_gain(m_pimpl->device.handle(), v);
    }
}

// Create stream with callback
void audio_device::create_stream_with_callback(
    void (*callback)(void* userdata, uint8_t* stream, int len),
    void* userdata) {
    
    if (!m_pimpl->device.valid()) {
        THROW_RUNTIME("No device available");
    }
    
    m_pimpl->stream = m_pimpl->device.manager()->create_stream(
        m_pimpl->device.handle(), m_pimpl->spec, callback, userdata
    );
    
    // Stream is already bound if using callback-based creation
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