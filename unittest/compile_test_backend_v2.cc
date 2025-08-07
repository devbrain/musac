// Simple compile test for audio_backend_v2 interface
// This file just verifies that the interface compiles without errors

#include <musac/sdk/audio_backend_v2.hh>

// Minimal mock to verify compilation
class test_backend : public musac::audio_backend_v2 {
public:
    void init() override {}
    void shutdown() override {}
    std::string get_name() const override { return "test"; }
    bool is_initialized() const override { return false; }
    
    std::vector<musac::device_info_v2> enumerate_devices(bool playback) override {
        return {};
    }
    
    musac::device_info_v2 get_default_device(bool playback) override {
        return {};
    }
    
    uint32_t open_device(const std::string& device_id, 
                         const musac::audio_spec& spec, 
                         musac::audio_spec& obtained_spec) override {
        return 0;
    }
    
    void close_device(uint32_t device_handle) override {}
    bool switch_device(uint32_t device_handle, const std::string& new_device_id) override { return false; }
    
    musac::audio_format get_device_format(uint32_t device_handle) override { 
        return musac::audio_format::s16; 
    }
    int get_device_frequency(uint32_t device_handle) override { return 44100; }
    int get_device_channels(uint32_t device_handle) override { return 2; }
    float get_device_gain(uint32_t device_handle) override { return 1.0f; }
    void set_device_gain(uint32_t device_handle, float gain) override {}
    
    bool pause_device(uint32_t device_handle) override { return false; }
    bool resume_device(uint32_t device_handle) override { return false; }
    bool is_device_paused(uint32_t device_handle) override { return false; }
    
    std::unique_ptr<musac::audio_stream_interface> create_stream(
        uint32_t device_handle,
        const musac::audio_spec& spec,
        void (*callback)(void* userdata, uint8_t* stream, int len),
        void* userdata) override {
        return nullptr;
    }
    
    bool supports_device_switching() const override { return false; }
    bool supports_recording() const override { return false; }
    int get_max_open_devices() const override { return 1; }
};

// Test that convenience methods work
void test_convenience_methods() {
    test_backend backend;
    
    // These should compile and work
    auto playback = backend.enumerate_playback_devices();
    auto recording = backend.enumerate_recording_devices();
    auto default_playback = backend.get_default_playback_device();
    auto default_recording = backend.get_default_recording_device();
    
    musac::audio_spec spec = {};
    auto stream = backend.create_stream(0, spec); // Convenience overload
}

int main() {
    test_convenience_methods();
    return 0;
}