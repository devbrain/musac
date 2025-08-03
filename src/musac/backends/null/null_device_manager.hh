#ifndef MUSAC_NULL_DEVICE_MANAGER_HH
#define MUSAC_NULL_DEVICE_MANAGER_HH

#include <musac/audio_device_interface.hh>
#include <musac/audio_stream_interface.hh>

namespace musac {

/**
 * Null device manager for testing and headless environments.
 */
class null_device_manager : public audio_device_interface {
public:
    std::vector<device_info> enumerate_devices(bool playback = true) override {
        device_info info;
        info.name = "Null Device";
        info.id = "null";
        info.is_default = true;
        info.channels = 2;
        info.sample_rate = 44100;
        return {info};
    }
    
    device_info get_default_device(bool playback = true) override {
        device_info info;
        info.name = "Null Device";
        info.id = "null";
        info.is_default = true;
        info.channels = 2;
        info.sample_rate = 44100;
        return info;
    }
    
    uint32_t open_device(const std::string& device_id, const audio_spec& spec, audio_spec& obtained_spec) override {
        obtained_spec = spec;
        return 1; // Always return handle 1
    }
    
    void close_device(uint32_t device_handle) override {}
    
    audio_format get_device_format(uint32_t device_handle) override {
        return audio_format::f32le;
    }
    
    int get_device_frequency(uint32_t device_handle) override {
        return 44100;
    }
    
    int get_device_channels(uint32_t device_handle) override {
        return 2;
    }
    
    float get_device_gain(uint32_t device_handle) override {
        return 1.0f;
    }
    
    void set_device_gain(uint32_t device_handle, float gain) override {}
    
    bool pause_device(uint32_t device_handle) override {
        m_device_paused = true;
        return true;
    }
    
    bool resume_device(uint32_t device_handle) override {
        m_device_paused = false;
        return true;
    }
    
    bool is_device_paused(uint32_t device_handle) override {
        return m_device_paused;
    }
    
    std::unique_ptr<audio_stream_interface> create_stream(
        uint32_t device_handle,
        const audio_spec& spec,
        void (*callback)(void* userdata, uint8_t* stream, int len),
        void* userdata
    ) override;

private:
    bool m_device_paused = false;
};

/**
 * Null audio stream that discards all data.
 */
class null_audio_stream : public audio_stream_interface {
public:
    bool put_data(const void* data, size_t size) override { return true; }
    size_t get_data(void* data, size_t size) override { return 0; }
    void clear() override {}
    bool pause() override { m_paused = true; return true; }
    bool resume() override { m_paused = false; return true; }
    bool is_paused() const override { return m_paused; }
    size_t get_queued_size() const override { return 0; }
    bool bind_to_device() override { return true; }
    void unbind_from_device() override {}
    
private:
    bool m_paused = false;
};

inline std::unique_ptr<audio_stream_interface> null_device_manager::create_stream(
    uint32_t device_handle,
    const audio_spec& spec,
    void (*callback)(void* userdata, uint8_t* stream, int len),
    void* userdata
) {
    return std::make_unique<null_audio_stream>();
}

} // namespace musac

#endif // MUSAC_NULL_DEVICE_MANAGER_HH