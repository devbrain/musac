// Simple compile test for audio_backend_v2 interface
// This file just verifies that the interface compiles without errors

#include <musac/sdk/audio_backend.hh>

// Minimal mock to verify compilation
class test_backend : public musac::audio_backend {
public:
    void init() override {}
    void shutdown() override {}
    std::string get_name() const override { return "test"; }
    bool is_initialized() const override { return false; }
    
    std::vector<musac::device_info> enumerate_devices(bool /*playback*/) override {
        return {};
    }
    
    musac::device_info get_default_device(bool /*playback*/) override {
        return {};
    }
    
    uint32_t open_device(const std::string& /*device_id*/, 
                         const musac::audio_spec& spec, 
                         musac::audio_spec& obtained_spec) override {
        obtained_spec = spec;  // Mock accepts any spec
        return 0;
    }
    
    void close_device(uint32_t /*device_handle*/) override {}
    
    musac::audio_format get_device_format(uint32_t /*device_handle*/) override { 
        return musac::audio_format::s16le; 
    }
    uint32_t get_device_frequency(uint32_t /*device_handle*/) override { return 44100; }
    uint8_t get_device_channels(uint32_t /*device_handle*/) override { return 2; }
    float get_device_gain(uint32_t /*device_handle*/) override { return 1.0f; }
    void set_device_gain(uint32_t /*device_handle*/, float /*gain*/) override {}
    
    bool pause_device(uint32_t /*device_handle*/) override { return false; }
    bool resume_device(uint32_t /*device_handle*/) override { return false; }
    bool is_device_paused(uint32_t /*device_handle*/) override { return false; }
    
    std::unique_ptr<musac::audio_stream_interface> create_stream(
        uint32_t /*device_handle*/,
        const musac::audio_spec& /*spec*/,
        void (*/*callback*/)(void* /*userdata*/, uint8_t* /*stream*/, int /*len*/),
        void* /*userdata*/) override {
        return nullptr;
    }
    
    bool supports_recording() const override { return false; }
    int get_max_open_devices() const override { return 1; }
};

// Test that convenience methods work
#include <doctest/doctest.h>

TEST_CASE("backend convenience methods compile") {
    test_backend backend;
    
    // These should compile and work
    auto playback = backend.enumerate_playback_devices();
    auto recording = backend.enumerate_recording_devices();
    auto default_playback = backend.get_default_playback_device();
    auto default_recording = backend.get_default_recording_device();
    
    musac::audio_spec spec = {};
    // The convenience overload requires default-constructible callback
    auto stream = backend.create_stream(0, spec, nullptr, nullptr);
    
    CHECK(playback.empty());
    CHECK(recording.empty());
}