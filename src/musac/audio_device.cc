#include <musac/audio_device.hh>
#include <musac/audio_device_interface.hh>
#include <musac/audio_stream_interface.hh>
#include <musac/audio_backend.hh>
#include <musac/stream.hh>
#include <musac/audio_device_data.hh>
#include <musac/sdk/from_float_converter.hh>
#include <failsafe/failsafe.hh>
#include <vector>
#include <memory>

namespace musac {

// Global device manager instance
static std::unique_ptr<audio_device_interface> s_device_manager;

// Initialize device manager if needed
static audio_device_interface* get_device_manager() {
    if (!s_device_manager) {
        // Need to include the factory header to get create_default_audio_device_manager
        extern std::unique_ptr<audio_device_interface> create_default_audio_device_manager();
        s_device_manager = create_default_audio_device_manager();
    }
    return s_device_manager.get();
}

// Close all audio devices (called from audio_system::done)
void close_audio_devices() {
    s_device_manager.reset();
}

// audio_hardware implementation
struct audio_hardware::impl {
    device_info info;
};

audio_hardware::audio_hardware() : m_pimpl(std::make_unique<impl>()) {}
audio_hardware::~audio_hardware() = default;
audio_hardware::audio_hardware(audio_hardware&&) noexcept = default;
audio_hardware& audio_hardware::operator=(audio_hardware&&) noexcept = default;

std::vector<audio_hardware> audio_hardware::enumerate(bool playback_devices) {
    auto* manager = get_device_manager();
    if (!manager) {
        return {};
    }
    
    auto devices = manager->enumerate_devices(playback_devices);
    std::vector<audio_hardware> result;
    
    for (const auto& dev : devices) {
        audio_hardware hw;
        hw.m_pimpl->info = dev;
        result.push_back(std::move(hw));
    }
    
    return result;
}

audio_hardware audio_hardware::get_default_device(bool playback_device) {
    auto* manager = get_device_manager();
    if (!manager) {
        THROW_RUNTIME("No audio device manager available");
    }
    
    audio_hardware hw;
    hw.m_pimpl->info = manager->get_default_device(playback_device);
    return hw;
}

std::string audio_hardware::get_device_name() const {
    return m_pimpl->info.name;
}

audio_format audio_hardware::get_format() const {
    // This is a limitation - we'd need to open the device to get its format
    // For now, return a default
    return audio_format::f32le;
}

int audio_hardware::get_channels() const {
    return m_pimpl->info.channels;
}

int audio_hardware::get_freq() const {
    return m_pimpl->info.sample_rate;
}

// audio_device implementation
struct audio_device::impl {
    uint32_t device_handle = 0;
    std::unique_ptr<audio_stream_interface> stream;
    audio_spec spec;
    audio_device_interface* manager = nullptr;
};

audio_device::audio_device(const audio_hardware& hw, const audio_spec* desired_spec)
    : m_pimpl(std::make_unique<impl>()) {
    
    m_pimpl->manager = get_device_manager();
    if (!m_pimpl->manager) {
        THROW_RUNTIME("No audio device manager available");
    }
    
    // Use hardware info to open device
    audio_spec spec;
    if (desired_spec) {
        spec = *desired_spec;
    } else {
        // Use hardware defaults
        spec.format = hw.get_format();
        spec.channels = hw.get_channels();
        spec.freq = hw.get_freq();
    }
    
    audio_spec obtained_spec;
    m_pimpl->device_handle = m_pimpl->manager->open_device(
        hw.m_pimpl->info.id, spec, obtained_spec
    );
    
    if (!m_pimpl->device_handle) {
        THROW_RUNTIME("Failed to open audio device");
    }
    
    m_pimpl->spec = obtained_spec;
}

audio_device::~audio_device() {
    if (m_pimpl && m_pimpl->manager && m_pimpl->device_handle) {
        m_pimpl->manager->close_device(m_pimpl->device_handle);
    }
}

audio_device::audio_device(audio_device&&) noexcept = default;

uint32_t audio_device::get_device_id() const {
    return m_pimpl->device_handle;
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
    if (m_pimpl->stream) {
        return m_pimpl->stream->pause();
    }
    return false;
}

bool audio_device::is_paused() const {
    if (m_pimpl->stream) {
        return m_pimpl->stream->is_paused();
    }
    return false;
}

bool audio_device::resume() {
    if (m_pimpl->stream) {
        return m_pimpl->stream->resume();
    }
    return false;
}

float audio_device::get_gain() const {
    if (m_pimpl->manager) {
        return m_pimpl->manager->get_device_gain(m_pimpl->device_handle);
    }
    return 0.0f;
}

void audio_device::set_gain(float v) {
    if (m_pimpl->manager) {
        m_pimpl->manager->set_device_gain(m_pimpl->device_handle, v);
    }
}

// Create stream with callback
void audio_device::create_stream_with_callback(
    void (*callback)(void* userdata, uint8_t* stream, int len),
    void* userdata) {
    
    if (!m_pimpl->manager) {
        THROW_RUNTIME("No device manager available");
    }
    
    m_pimpl->stream = m_pimpl->manager->create_stream(
        m_pimpl->device_handle, m_pimpl->spec, callback, userdata
    );
    
    if (!m_pimpl->stream) {
        THROW_RUNTIME("Failed to create audio stream");
    }
    
    // Stream is already bound if using callback-based creation
}

audio_stream audio_device::create_stream(audio_source&& audio_src) {
    // Set up audio device data for the stream system
    audio_device_data aud;
    aud.m_audio_spec = m_pimpl->spec;
    // m_stream is no longer used - we use direct conversion instead of SDL streams
    aud.m_frame_size = 4096; // Default frame size
    aud.m_sample_converter = get_from_float_converter(m_pimpl->spec.format);
    
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