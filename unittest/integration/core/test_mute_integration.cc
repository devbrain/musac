/**
 * @file test_mute_integration.cc
 * @brief Integration tests for global mute/unmute functionality
 * 
 * Test Coverage:
 * - Backend-level mute support (SDL2/SDL3)
 * - Mixer-level mute fallback
 * - Device API mute methods
 * - Mute affecting audio output
 * - Interaction with stream playback
 * 
 * Dependencies:
 * - Requires audio backend (SDL2/SDL3)
 * - Tests with real audio system
 */

#include <doctest/doctest.h>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <musac_backends/sdl2/sdl2_backend.hh>
#include <musac_backends/sdl3/sdl3_backend.hh>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>
#include <cmath>

using namespace musac;

// Helper to create a simple tone generator source
class tone_generator_source {
    sample_rate_t m_sample_rate;
    float m_frequency;
    size_t m_position = 0;
    
public:
    tone_generator_source(sample_rate_t rate, float freq)
        : m_sample_rate(rate), m_frequency(freq) {}
    
    audio_source create() {
        audio_source source;
        source.m_spec.freq = m_sample_rate;
        source.m_spec.format = audio_format::f32le;
        source.m_spec.channels = 2;
        
        // Generate 1 second of tone
        size_t samples = m_sample_rate * 2; // stereo
        std::vector<float> data(samples);
        
        for (size_t i = 0; i < samples; i += 2) {
            float sample = std::sin(2.0f * M_PI * m_frequency * (i/2) / m_sample_rate) * 0.5f;
            data[i] = sample;     // left
            data[i+1] = sample;   // right
        }
        
        // Create buffer from data
        source.m_buffer = buffer<uint8_t>(data.size() * sizeof(float));
        std::memcpy(source.m_buffer.data(), data.data(), data.size() * sizeof(float));
        
        return source;
    }
};

TEST_SUITE("Mute::Integration") {
    
    TEST_CASE("SDL2_backend_mute_integration") {
        auto backend = create_sdl2_backend();
        REQUIRE(backend != nullptr);
        backend->init();
        
        SUBCASE("device_level_mute_with_playback") {
            auto device = audio_device::open_default_device(backend);
            
            // Verify backend supports mute
            CHECK(device.has_hardware_mute() == true);
            
            // Create and play a tone
            tone_generator_source tone_gen(44100, 440.0f); // A4 note
            auto source = tone_gen.create();
            auto stream = device.create_stream(std::move(source));
            
            stream.open();
            stream.play();
            CHECK(stream.is_playing() == true);
            
            // Let it play briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Mute the device
            device.mute_all();
            CHECK(device.is_all_muted() == true);
            
            // Stream should still be playing but muted
            CHECK(stream.is_playing() == true);
            
            // Wait a bit while muted
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Unmute
            device.unmute_all();
            CHECK(device.is_all_muted() == false);
            
            // Let it play again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            stream.stop();
        }
        
        SUBCASE("mute_multiple_streams") {
            auto device = audio_device::open_default_device(backend);
            
            // Create multiple streams with different frequencies
            tone_generator_source tone1(44100, 440.0f);  // A4
            tone_generator_source tone2(44100, 554.37f); // C#5
            tone_generator_source tone3(44100, 659.25f); // E5
            
            auto source1 = tone1.create();
            auto source2 = tone2.create();
            auto source3 = tone3.create();
            
            auto stream1 = device.create_stream(std::move(source1));
            auto stream2 = device.create_stream(std::move(source2));
            auto stream3 = device.create_stream(std::move(source3));
            
            stream1.open();
            stream2.open();
            stream3.open();
            
            stream1.play();
            stream2.play();
            stream3.play();
            
            // All playing
            CHECK(stream1.is_playing() == true);
            CHECK(stream2.is_playing() == true);
            CHECK(stream3.is_playing() == true);
            
            // Mute all
            device.mute_all();
            CHECK(device.is_all_muted() == true);
            
            // All still playing but muted
            CHECK(stream1.is_playing() == true);
            CHECK(stream2.is_playing() == true);
            CHECK(stream3.is_playing() == true);
            
            // Unmute
            device.unmute_all();
            CHECK(device.is_all_muted() == false);
            
            // Stop all
            stream1.stop();
            stream2.stop();
            stream3.stop();
        }
        
        backend->shutdown();
    }
    
    TEST_CASE("SDL3_backend_mute_integration") {
        auto backend = create_sdl3_backend();
        REQUIRE(backend != nullptr);
        backend->init();
        
        SUBCASE("device_level_mute_with_playback") {
            auto device = audio_device::open_default_device(backend);
            
            // Verify backend supports mute
            CHECK(device.has_hardware_mute() == true);
            
            // Create and play a tone
            tone_generator_source tone_gen(44100, 440.0f); // A4 note
            auto source = tone_gen.create();
            auto stream = device.create_stream(std::move(source));
            
            stream.open();
            stream.play();
            CHECK(stream.is_playing() == true);
            
            // Let it play briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Mute the device
            device.mute_all();
            CHECK(device.is_all_muted() == true);
            
            // Stream should still be playing but muted
            CHECK(stream.is_playing() == true);
            
            // Wait a bit while muted
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Unmute
            device.unmute_all();
            CHECK(device.is_all_muted() == false);
            
            // Let it play again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            stream.stop();
        }
        
        backend->shutdown();
    }
    
    TEST_CASE("mixer_fallback_mute_integration") {
        // Use a mock backend that doesn't support mute to test fallback
        class no_mute_backend : public audio_backend {
        public:
            void init() override {}
            void shutdown() override {}
            std::string get_name() const override { return "NoMute"; }
            bool is_initialized() const override { return true; }
            std::vector<device_info> enumerate_devices(bool) override { 
                return {{
                    "Default", "0", true, 2, 44100
                }};
            }
            device_info get_default_device(bool) override { 
                return {"Default", "0", true, 2, 44100};
            }
            uint32_t open_device(const std::string&, const audio_spec& spec, audio_spec& obtained) override {
                obtained = spec;
                return 1;
            }
            void close_device(uint32_t) override {}
            audio_format get_device_format(uint32_t) override { return audio_format::s16le; }
            sample_rate_t get_device_frequency(uint32_t) override { return 44100; }
            channels_t get_device_channels(uint32_t) override { return 2; }
            float get_device_gain(uint32_t) override { return 1.0f; }
            void set_device_gain(uint32_t, float) override {}
            bool pause_device(uint32_t) override { return true; }
            bool resume_device(uint32_t) override { return true; }
            bool is_device_paused(uint32_t) override { return false; }
            // No mute support - use defaults from base class
            std::unique_ptr<audio_stream_interface> create_stream(
                uint32_t, const audio_spec&, 
                void (*)(void*, uint8_t*, int), void*) override {
                return nullptr;
            }
            bool supports_recording() const override { return false; }
            int get_max_open_devices() const override { return 1; }
        };
        
        auto backend = std::make_shared<no_mute_backend>();
        backend->init();
        
        SUBCASE("fallback_to_mixer_mute") {
            auto device = audio_device::open_default_device(backend);
            
            // Backend doesn't support mute
            CHECK(device.has_hardware_mute() == false);
            
            // Initially not muted
            CHECK(device.is_all_muted() == false);
            
            // Mute should fallback to mixer
            device.mute_all();
            CHECK(device.is_all_muted() == true);
            
            // Unmute
            device.unmute_all();
            CHECK(device.is_all_muted() == false);
        }
    }
}