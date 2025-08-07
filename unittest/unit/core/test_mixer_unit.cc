/**
 * @file test_mixer_unit.cc
 * @brief Unit tests for audio mixer functionality using mocks
 * 
 * Test Coverage:
 * - Mixer stream management (add/remove)
 * - Mixing multiple audio streams
 * - Volume and gain control in mixing
 * - Buffer management and optimization
 * - Thread safety of mixer operations
 * - Edge cases (empty mixer, single stream, many streams)
 * 
 * Dependencies:
 * - mock_stream for stream simulation
 * - No hardware dependencies
 * 
 * @note Some mixer functionality may be internal and not directly testable
 */

#include <doctest/doctest.h>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include "../../mock_backends.hh"
#include "../../mock_components.hh"
#include "../../test_fixtures.hh"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

using namespace musac;
using namespace musac::test;

TEST_SUITE("Mixer::Unit") {
    
    TEST_CASE("should_handle_single_stream_mixing") {
        // Arrange
        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        backend->init();
        auto device = audio_device::open_default_device(backend);
        
        SUBCASE("mix_single_stream") {
            // Create and play a single stream
            auto source = create_mock_source(44100);
            auto stream = device.create_stream(std::move(*source));
            stream.open();
            stream.set_volume(0.5f);
            stream.play();
            
            // Assert: Stream should be playing
            CHECK(stream.is_playing());
            CHECK(stream.volume() == 0.5f);
            
            // The mixer should handle this stream internally
            // Actual mixing happens in the audio callback
        }
        
        SUBCASE("single_stream_volume_changes") {
            auto source = create_mock_source(44100);
            auto stream = device.create_stream(std::move(*source));
            stream.open();
            stream.play();
            
            // Change volume while playing
            for (float vol = 0.0f; vol <= 1.0f; vol += 0.1f) {
                CHECK_NOTHROW(stream.set_volume(vol));
                CHECK(doctest::Approx(stream.volume()) == vol);
            }
        }
    }
    
    TEST_CASE("should_handle_multiple_streams_mixing") {
        // Arrange
        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        backend->init();
        auto device = audio_device::open_default_device(backend);
        
        SUBCASE("mix_two_streams") {
            // Create two streams
            auto source1 = create_mock_source(44100);
            auto source2 = create_mock_source(44100);
            
            auto stream1 = device.create_stream(std::move(*source1));
            auto stream2 = device.create_stream(std::move(*source2));
            
            stream1.open();
            stream2.open();
            
            // Play both streams with different volumes
            stream1.set_volume(0.7f);
            stream2.set_volume(0.3f);
            
            stream1.play();
            stream2.play();
            
            // Assert: Both should be playing
            CHECK(stream1.is_playing());
            CHECK(stream2.is_playing());
            CHECK(stream1.volume() == 0.7f);
            CHECK(stream2.volume() == 0.3f);
        }
        
        SUBCASE("mix_many_streams") {
            // Create multiple streams
            std::vector<audio_stream> streams;
            const int STREAM_COUNT = 10;
            
            for (int i = 0; i < STREAM_COUNT; ++i) {
                auto source = create_mock_source(44100);
                auto stream = device.create_stream(std::move(*source));
                stream.open();
                stream.set_volume(1.0f / STREAM_COUNT); // Distribute volume
                stream.play();
                streams.push_back(std::move(stream));
            }
            
            // Assert: All streams should be playing
            for (const auto& stream : streams) {
                CHECK(stream.is_playing());
            }
        }
    }
    
    TEST_CASE("should_handle_dynamic_stream_management") {
        // Arrange
        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        backend->init();
        auto device = audio_device::open_default_device(backend);
        
        SUBCASE("add_and_remove_streams_dynamically") {
            std::vector<std::unique_ptr<audio_stream>> streams;
            
            // Add streams one by one
            for (int i = 0; i < 5; ++i) {
                auto source = create_mock_source(44100);
                auto stream = std::make_unique<audio_stream>(
                    device.create_stream(std::move(*source))
                );
                stream->open();
                stream->play();
                CHECK(stream->is_playing());
                streams.push_back(std::move(stream));
                
                // Small delay to simulate real usage
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            // Remove streams one by one
            while (!streams.empty()) {
                streams.pop_back(); // Destructor stops and removes from mixer
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            // All streams removed, mixer should handle empty state
        }
        
        SUBCASE("replace_streams_while_playing") {
            // Start with one stream
            auto source1 = create_mock_source(44100);
            auto stream1 = device.create_stream(std::move(*source1));
            stream1.open();
            stream1.play();
            
            // Replace with another stream
            stream1.stop();
            auto source2 = create_mock_source(22050);
            auto stream2 = device.create_stream(std::move(*source2));
            stream2.open();
            stream2.play();
            
            CHECK(!stream1.is_playing());
            CHECK(stream2.is_playing());
        }
    }
    
    TEST_CASE("should_handle_stream_state_changes") {
        // Arrange
        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        backend->init();
        auto device = audio_device::open_default_device(backend);
        
        SUBCASE("pause_individual_streams") {
            // Create multiple streams
            auto source1 = create_mock_source(44100);
            auto source2 = create_mock_source(44100);
            auto source3 = create_mock_source(44100);
            
            auto stream1 = device.create_stream(std::move(*source1));
            auto stream2 = device.create_stream(std::move(*source2));
            auto stream3 = device.create_stream(std::move(*source3));
            
            stream1.open();
            stream2.open();
            stream3.open();
            
            stream1.play();
            stream2.play();
            stream3.play();
            
            // Pause middle stream
            stream2.pause();
            
            // Assert: Mixer should handle mixed states
            CHECK(stream1.is_playing());
            CHECK(stream2.is_paused());
            CHECK(stream3.is_playing());
            
            // Resume
            stream2.resume();
            CHECK(stream2.is_playing());
        }
        
        SUBCASE("mute_individual_streams") {
            // Create streams
            auto source1 = create_mock_source(44100);
            auto source2 = create_mock_source(44100);
            
            auto stream1 = device.create_stream(std::move(*source1));
            auto stream2 = device.create_stream(std::move(*source2));
            
            stream1.open();
            stream2.open();
            stream1.play();
            stream2.play();
            
            // Mute one stream
            stream1.mute();
            
            CHECK(stream1.is_muted());
            CHECK(!stream2.is_muted());
            // Mute doesn't change volume value, just mutes output
            // stream2 should keep its volume
        }
    }
    
    TEST_CASE("should_handle_concurrent_mixer_operations") {
        /**
         * Test thread safety of mixer with concurrent operations
         */
        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        backend->init();
        auto device = audio_device::open_default_device(backend);
        
        SUBCASE("concurrent_stream_creation") {
            std::vector<std::thread> threads;
            std::vector<std::unique_ptr<audio_stream>> streams;
            std::mutex streams_mutex;
            
            // Launch threads that create streams
            for (int t = 0; t < 4; ++t) {
                threads.emplace_back([&device, &streams, &streams_mutex]() {
                    for (int i = 0; i < 5; ++i) {
                        auto source = create_mock_source(44100);
                        auto stream = std::make_unique<audio_stream>(
                            device.create_stream(std::move(*source))
                        );
                        stream->open();
                        stream->play();
                        
                        std::lock_guard<std::mutex> lock(streams_mutex);
                        streams.push_back(std::move(stream));
                    }
                });
            }
            
            // Wait for all threads
            for (auto& t : threads) {
                t.join();
            }
            
            // Assert: All streams created
            CHECK(streams.size() == 20);
            
            // Verify all are playing
            for (const auto& stream : streams) {
                CHECK(stream->is_playing());
            }
        }
        
        SUBCASE("concurrent_volume_changes") {
            // Create streams
            std::vector<audio_stream> streams;
            for (int i = 0; i < 5; ++i) {
                auto source = create_mock_source(44100);
                auto stream = device.create_stream(std::move(*source));
                stream.open();
                stream.play();
                streams.push_back(std::move(stream));
            }
            
            // Launch threads that change volumes
            std::vector<std::thread> threads;
            std::atomic<int> operations{0};
            
            for (size_t i = 0; i < streams.size(); ++i) {
                threads.emplace_back([&streams, i, &operations]() {
                    for (int j = 0; j < 100; ++j) {
                        float volume = static_cast<float>(j % 11) / 10.0f;
                        streams[i].set_volume(volume);
                        operations++;
                    }
                });
            }
            
            // Wait for threads
            for (auto& t : threads) {
                t.join();
            }
            
            CHECK(operations == 500);
        }
        
        SUBCASE("concurrent_play_pause_operations") {
            // Create streams
            std::vector<audio_stream> streams;
            for (int i = 0; i < 8; ++i) {
                auto source = create_mock_source(44100 * 2);
                auto stream = device.create_stream(std::move(*source));
                stream.open();
                streams.push_back(std::move(stream));
            }
            
            // Launch threads with different operations
            std::vector<std::thread> threads;
            std::atomic<bool> stop{false};
            
            // Play/pause threads
            for (size_t i = 0; i < streams.size() / 2; ++i) {
                threads.emplace_back([&streams, i, &stop]() {
                    while (!stop) {
                        streams[i].play();
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        streams[i].pause();
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                });
            }
            
            // Volume change threads
            for (size_t i = streams.size() / 2; i < streams.size(); ++i) {
                threads.emplace_back([&streams, i, &stop]() {
                    streams[i].play();
                    while (!stop) {
                        float vol = static_cast<float>(rand() % 101) / 100.0f;
                        streams[i].set_volume(vol);
                        std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    }
                });
            }
            
            // Let it run
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            stop = true;
            
            // Wait for threads
            for (auto& t : threads) {
                t.join();
            }
            
            // Mixer should still be functional
            for (auto& stream : streams) {
                CHECK_NOTHROW(stream.stop());
            }
        }
    }
    
    TEST_CASE("should_handle_edge_cases") {
        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        backend->init();
        auto device = audio_device::open_default_device(backend);
        
        SUBCASE("empty_mixer") {
            // No streams added - mixer should handle empty state
            // This is implicitly tested by having a device with no streams
            
            // Device operations should still work
            CHECK_NOTHROW(device.pause());
            CHECK_NOTHROW(device.resume());
            CHECK_NOTHROW(device.set_gain(0.5f));
        }
        
        SUBCASE("all_streams_paused") {
            // Create streams but pause all
            std::vector<audio_stream> streams;
            for (int i = 0; i < 3; ++i) {
                auto source = create_mock_source(44100);
                auto stream = device.create_stream(std::move(*source));
                stream.open();
                stream.play();
                stream.pause();
                streams.push_back(std::move(stream));
            }
            
            // All paused - mixer should handle this
            for (const auto& stream : streams) {
                CHECK(stream.is_paused());
            }
        }
        
        SUBCASE("all_streams_muted") {
            // Create streams but mute all
            std::vector<audio_stream> streams;
            for (int i = 0; i < 3; ++i) {
                auto source = create_mock_source(44100);
                auto stream = device.create_stream(std::move(*source));
                stream.open();
                stream.play();
                stream.mute();
                streams.push_back(std::move(stream));
            }
            
            // All muted - effective silence
            for (const auto& stream : streams) {
                CHECK(stream.is_muted());
                // Muted streams keep their volume value
            }
        }
        
        SUBCASE("very_short_streams") {
            // Create streams with minimal audio data
            for (int i = 0; i < 5; ++i) {
                auto source = create_mock_source(100); // Very short
                auto stream = device.create_stream(std::move(*source));
                stream.open();
                stream.play();
                
                // Stream should complete quickly
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        
        SUBCASE("zero_volume_streams") {
            // Create streams with zero volume
            auto source1 = create_mock_source(44100);
            auto source2 = create_mock_source(44100);
            
            auto stream1 = device.create_stream(std::move(*source1));
            auto stream2 = device.create_stream(std::move(*source2));
            
            stream1.open();
            stream2.open();
            
            stream1.set_volume(0.0f);
            stream2.set_volume(0.0f);
            
            stream1.play();
            stream2.play();
            
            // Both playing but silent
            CHECK(stream1.is_playing());
            CHECK(stream2.is_playing());
            CHECK(stream1.volume() == 0.0f);
            CHECK(stream2.volume() == 0.0f);
        }
    }
}