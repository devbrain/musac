#include <doctest/doctest.h>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include "../test_helpers.hh"
#include <thread>
#include <chrono>
#include <vector>
#include <memory>

TEST_CASE("audio_stream move semantics") {
    using namespace musac;
    using namespace musac::test;
    
    SUBCASE("move constructor basic") {
        try {
            // Open audio device
            auto device = audio_device::open_default_device();
            
            // Create a stream using device
            auto source = create_mock_source(8000);  // Short duration
            audio_stream stream1 = device.create_stream(std::move(*source));
            stream1.open();
            stream1.play();
            
            // Check that stream is playing before move
            CHECK(stream1.is_playing());
            
            // Move the stream
            audio_stream stream2(std::move(stream1));
            
            // The moved-to stream should work correctly
            CHECK(stream2.is_playing());
            
            // Give audio callback time to run with moved stream
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Should not crash - the mixer has been updated
            stream2.stop();
            CHECK(!stream2.is_playing());
            
        } catch (const std::exception& e) {
            // Device might not be available in test environment
            INFO("Exception: " << e.what());
            WARN("Skipping test - audio device not available");
        }
    }
    
    SUBCASE("move assignment basic") {
        try {
            // Open audio device
            auto device = audio_device::open_default_device();
            
            // Create and play a stream
            auto source1 = create_mock_source(8000);
            audio_stream stream1 = device.create_stream(std::move(*source1));
            stream1.open();
            stream1.play();
            CHECK(stream1.is_playing());
            
            // Create another stream
            auto source2 = create_mock_source(8000);
            audio_stream stream2 = device.create_stream(std::move(*source2));
            
            // Move assign
            stream2 = std::move(stream1);
            
            // The moved-to stream should work correctly
            CHECK(stream2.is_playing());
            
            // Give audio callback time to run with moved stream
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Should not crash - the mixer has been updated
            stream2.stop();
            CHECK(!stream2.is_playing());
            
        } catch (const std::exception& e) {
            INFO("Exception: " << e.what());
            WARN("Skipping test - audio device not available");
        }
    }
    
    SUBCASE("moved streams in containers") {
        try {
            // Open audio device
            auto device = audio_device::open_default_device();
            
            // Create streams in a vector
            std::vector<audio_stream> streams;
            streams.reserve(3);  // Avoid reallocation
            
            for (int i = 0; i < 3; ++i) {
                auto source = create_mock_source(8000);
                audio_stream stream = device.create_stream(std::move(*source));
                stream.open();
                stream.play();
                CHECK(stream.is_playing());
                streams.push_back(std::move(stream));
            }
            
            // Give audio callback time to run
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // All streams should work correctly after move
            for (auto& stream : streams) {
                CHECK(stream.is_playing());
                stream.stop();
                CHECK(!stream.is_playing());
            }
            
        } catch (const std::exception& e) {
            INFO("Exception: " << e.what());
            WARN("Skipping test - audio device not available");
        }
    }
    
    SUBCASE("stress test - multiple moves during playback") {
        try {
            // Open audio device
            auto device = audio_device::open_default_device();
            
            // Create initial stream
            auto source = create_mock_source(44100);  // 1 second
            auto stream = std::make_unique<audio_stream>(
                device.create_stream(std::move(*source))
            );
            stream->open();
            stream->play();
            CHECK(stream->is_playing());
            
            // Move stream multiple times while playing
            for (int i = 0; i < 5; ++i) {
                auto new_stream = std::make_unique<audio_stream>(std::move(*stream));
                stream = std::move(new_stream);
                
                // Brief pause to let audio callback run
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                
                // Should still be playing
                CHECK(stream->is_playing());
            }
            
            // Final stop
            stream->stop();
            CHECK(!stream->is_playing());
            
        } catch (const std::exception& e) {
            INFO("Exception: " << e.what());
            WARN("Skipping test - audio device not available");
        }
    }
    
    SUBCASE("move return from function") {
        try {
            // Open audio device
            auto device = audio_device::open_default_device();
            
            // Function that returns a stream by move
            auto create_playing_stream = [&device]() -> audio_stream {
                auto source = create_mock_source(8000);
                audio_stream stream = device.create_stream(std::move(*source));
                stream.open();
                stream.play();
                return stream;  // Return by move
            };
            
            // Get stream from function
            audio_stream stream = create_playing_stream();
            
            // Should work correctly after move return
            CHECK(stream.is_playing());
            
            // Give audio callback time to run
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Stop and verify
            stream.stop();
            CHECK(!stream.is_playing());
            
        } catch (const std::exception& e) {
            INFO("Exception: " << e.what());
            WARN("Skipping test - audio device not available");
        }
    }
}