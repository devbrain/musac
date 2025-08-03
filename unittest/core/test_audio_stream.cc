#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <musac/sdk/buffer.hh>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <mutex>
#include <cmath>
#include <memory>
#include "../test_helpers.hh"

namespace musac::test {

TEST_SUITE("audio_stream") {
    // Test fixture to ensure proper initialization/cleanup
    struct audio_test_fixture {
        audio_test_fixture() {
            audio_system::init();
        }
        
        ~audio_test_fixture() {
            // Small delay to ensure audio callbacks complete
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            audio_system::done();
        }
    };
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream construction and destruction") {
        auto device = audio_device::open_default_device();
        auto source = create_mock_source();
        
        {
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
        }
        // Stream should be properly destroyed here
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream play and stop") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        SUBCASE("immediate stop") {
            auto source = create_mock_source(44100); // 1 second
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
            
            CHECK(stream.play());
            CHECK(stream.is_playing());
            stream.stop();
            CHECK_FALSE(stream.is_playing());
        }
        
        SUBCASE("stop with fade") {
            auto source = create_mock_source(44100); // 1 second
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
            
            CHECK(stream.play());
            CHECK(stream.is_playing());
            
            // Give SDL time to start audio callbacks
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            stream.stop(std::chrono::milliseconds(200));
            // Still playing during fade
            CHECK(stream.is_playing());
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            CHECK_FALSE(stream.is_playing());
        }
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream pause and resume") {
        auto device = audio_device::open_default_device();
        auto source = create_mock_source();
        
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        device.resume();
        
        CHECK(stream.play());
        CHECK(stream.is_playing());
        CHECK_FALSE(stream.is_paused());
        
        stream.pause();
        CHECK(stream.is_paused());
        
        stream.resume();
        CHECK_FALSE(stream.is_paused());
        CHECK(stream.is_playing());
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream finish callback") {
        auto device = audio_device::open_default_device();
        auto source = create_mock_source(4410); // 0.1 second at 44.1kHz
        
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        device.resume();
        
        std::atomic<bool> callback_called{false};
        std::atomic<int> callback_count{0};
        
        stream.set_finish_callback([&callback_called, &callback_count](audio_stream& /*s*/) {
            callback_called = true;
            callback_count++;
        });
        
        CHECK(stream.play());
        
        // Wait for playback to finish
        auto start = std::chrono::steady_clock::now();
        while (!callback_called && std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        CHECK(callback_called);
        CHECK(callback_count == 1);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream loop callback") {
        auto device = audio_device::open_default_device();
        auto source = create_mock_source(4410); // 0.1 second
        
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        device.resume();
        
        std::atomic<int> loop_count{0};
        
        stream.set_loop_callback([&loop_count](audio_stream& /*s*/) {
            loop_count++;
        });
        
        CHECK(stream.play(3)); // Play 3 times
        
        // Wait for loops
        auto start = std::chrono::steady_clock::now();
        while (loop_count < 2 && std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        CHECK(loop_count >= 2); // Should have looped at least twice
        // Note: Cannot check internal rewind_count after move
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream volume control") {
        auto device = audio_device::open_default_device();
        auto source = create_mock_source();
        
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        
        CHECK(stream.volume() == doctest::Approx(1.0f));
        
        stream.set_volume(0.5f);
        CHECK(stream.volume() == doctest::Approx(0.5f));
        
        stream.set_volume(-0.5f); // Should clamp to 0
        CHECK(stream.volume() == doctest::Approx(0.0f));
        
        stream.set_volume(2.0f); // No upper limit enforced in set_volume
        CHECK(stream.volume() == doctest::Approx(2.0f));
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream stereo position") {
        auto device = audio_device::open_default_device();
        auto source = create_mock_source();
        
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        
        CHECK(stream.get_stereo_position() == doctest::Approx(0.0f));
        
        stream.set_stereo_position(-1.0f); // Full left
        CHECK(stream.get_stereo_position() == doctest::Approx(-1.0f));
        
        stream.set_stereo_position(1.0f); // Full right
        CHECK(stream.get_stereo_position() == doctest::Approx(1.0f));
        
        stream.set_stereo_position(2.0f); // Should clamp
        CHECK(stream.get_stereo_position() == doctest::Approx(1.0f));
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream mute/unmute") {
        auto device = audio_device::open_default_device();
        auto source = create_mock_source();
        
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        
        CHECK_FALSE(stream.is_muted());
        
        stream.mute();
        CHECK(stream.is_muted());
        
        stream.unmute();
        CHECK_FALSE(stream.is_muted());
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream seeking") {
        auto device = audio_device::open_default_device();
        auto source = create_mock_source(44100); // 1 second
        
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        
        CHECK(stream.duration() == std::chrono::seconds(1));
        
        CHECK(stream.seek_to_time(std::chrono::milliseconds(500)));
        // Note: Cannot verify internal position after move
        
        CHECK(stream.rewind());
        // Note: Cannot verify internal position after move
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "concurrent stream operations") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        std::vector<std::unique_ptr<audio_stream>> streams;
        std::atomic<int> total_callbacks{0};
        
        // Create multiple streams
        for (int i = 0; i < 5; ++i) {
            auto source = create_mock_source(2205); // 0.05 second
            auto stream_ptr = std::make_unique<audio_stream>(device.create_stream(std::move(*source)));
            
            REQUIRE_NOTHROW(stream_ptr->open());
            stream_ptr->set_finish_callback([&total_callbacks](audio_stream& /*s*/) {
                total_callbacks++;
            });
            
            streams.push_back(std::move(stream_ptr));
        }
        
        // Play all streams
        for (auto& stream : streams) {
            stream->play();
        }
        
        // Wait for all to finish
        auto start = std::chrono::steady_clock::now();
        while (total_callbacks < 5 && std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        CHECK(total_callbacks == 5);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream destruction during playback") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        std::atomic<bool> callback_called{false};
        
        {
            auto source = create_mock_source(44100); // 1 second
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
            
            stream.set_finish_callback([&callback_called](audio_stream& /*s*/) {
                callback_called = true;
            });
            
            stream.play();
            CHECK(stream.is_playing());
            
            // Let it play for a bit
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Stream destroyed while playing
        }
        
        // Callback should not be called after destruction
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CHECK_FALSE(callback_called);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "callback removal") {
        auto device = audio_device::open_default_device();
        auto source = create_mock_source(4410); // 0.1 second
        
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        device.resume();
        
        std::atomic<bool> finish_called{false};
        std::atomic<bool> loop_called{false};
        
        stream.set_finish_callback([&finish_called](audio_stream& /*s*/) {
            finish_called = true;
        });
        
        stream.set_loop_callback([&loop_called](audio_stream& /*s*/) {
            loop_called = true;
        });
        
        // Remove callbacks before playing
        stream.remove_finish_callback();
        stream.remove_loop_callback();
        
        stream.play(2); // Play twice to test loop
        
        // Wait
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        CHECK_FALSE(finish_called);
        CHECK_FALSE(loop_called);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "rapid play/stop cycles") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        auto source = create_mock_source(44100);
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        
        // Rapid play/stop cycles
        for (int i = 0; i < 10; ++i) {
            CHECK(stream.play());
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            stream.stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        
        // Should handle rapid state changes without issues
        CHECK_FALSE(stream.is_playing());
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream state machine transitions") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        auto source = create_mock_source(44100 * 5); // 5 seconds
        auto stream = device.create_stream(std::move(*source));
        
        // State: Uninitialized -> Initialized
        REQUIRE_NOTHROW(stream.open());
        CHECK_FALSE(stream.is_playing());
        CHECK_FALSE(stream.is_paused());
        
        // State: Initialized -> Playing
        CHECK(stream.play());
        CHECK(stream.is_playing());
        CHECK_FALSE(stream.is_paused());
        
        // State: Playing -> Paused
        stream.pause();
        CHECK_FALSE(stream.is_playing());
        CHECK(stream.is_paused());
        
        // State: Paused -> Playing (resume)
        stream.resume();
        CHECK(stream.is_playing());
        CHECK_FALSE(stream.is_paused());
        
        // State: Playing -> Stopped
        stream.stop();
        CHECK_FALSE(stream.is_playing());
        CHECK_FALSE(stream.is_paused());
        
        // State: Stopped -> Playing (can play again)
        CHECK(stream.play());
        CHECK(stream.is_playing());
        
        // Test pause with fade
        stream.pause(std::chrono::milliseconds(50));
        // Should still be playing during fade
        CHECK(stream.is_playing());
        CHECK_FALSE(stream.is_paused());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CHECK(stream.is_paused());
        CHECK_FALSE(stream.is_playing());
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "callback thread safety stress test") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        const int num_streams = 10;
        std::vector<std::unique_ptr<audio_stream>> streams;
        std::atomic<int> callback_count{0};
        std::mutex callback_mutex;
        std::vector<std::pair<int, std::string>> callback_log;
        
        for (int i = 0; i < num_streams; ++i) {
            auto source = create_mock_source(2205); // 0.05 second
            auto stream = std::make_unique<audio_stream>(device.create_stream(std::move(*source)));
            REQUIRE_NOTHROW(stream->open());
            
            int stream_id = i;
            stream->set_finish_callback([&callback_count, &callback_mutex, &callback_log, stream_id](audio_stream& /*s*/) {
                callback_count++;
                std::lock_guard<std::mutex> lock(callback_mutex);
                callback_log.push_back({stream_id, "finish"});
            });
            
            stream->set_loop_callback([&callback_mutex, &callback_log, stream_id](audio_stream& /*s*/) {
                std::lock_guard<std::mutex> lock(callback_mutex);
                callback_log.push_back({stream_id, "loop"});
            });
            
            streams.push_back(std::move(stream));
        }
        
        // Play with different configurations
        for (int i = 0; i < num_streams; ++i) {
            if (i % 3 == 0) {
                streams[static_cast<size_t>(i)]->play(3); // Loop 3 times
            } else {
                streams[static_cast<size_t>(i)]->play(1); // Play once
            }
        }
        
        // Wait for completion
        auto start = std::chrono::steady_clock::now();
        while (callback_count < num_streams && 
               std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        CHECK(callback_count == num_streams);
        
        // Verify callback order makes sense
        std::lock_guard<std::mutex> lock(callback_mutex);
        for (const auto& [id, type] : callback_log) {
            CHECK(id >= 0);
            CHECK(id < num_streams);
            CHECK((type == "finish" || type == "loop"));
        }
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream cleanup with pending callbacks") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        std::atomic<int> callback_executions{0};
        std::atomic<bool> stream_destroyed{false};
        
        {
            auto source = create_mock_source(2205); // 0.05 second
            auto stream = device.create_stream(std::move(*source));
            REQUIRE_NOTHROW(stream.open());
            
            stream.set_finish_callback([&callback_executions, &stream_destroyed](audio_stream& /*s*/) {
                // Simulate slow callback
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                callback_executions++;
                // Check if we can still access the stream
                CHECK_FALSE(stream_destroyed.load());
            });
            
            stream.play();
            
            // Give it time to start
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Stream will be destroyed here
        }
        
        stream_destroyed = true;
        
        // The destructor should have waited for callback to complete
        // or prevented it from executing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Callback should either have executed before destruction
        // or been prevented from executing
        CHECK(callback_executions <= 1);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "pause/resume with callbacks") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        auto source = create_mock_source(44100); // 1 second
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        
        std::atomic<bool> finish_called{false};
        std::atomic<bool> was_playing_during_pause{false};
        
        stream.set_finish_callback([&finish_called](audio_stream& /*s*/) {
            finish_called = true;
        });
        
        stream.play();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Pause
        stream.pause();
        CHECK(stream.is_paused());
        CHECK_FALSE(stream.is_playing());
        
        // Wait a bit - stream should remain paused
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        CHECK(stream.is_paused());
        
        // Resume
        stream.resume();
        CHECK_FALSE(stream.is_paused());
        CHECK(stream.is_playing());
        
        // Should continue and eventually finish
        auto start = std::chrono::steady_clock::now();
        while (!finish_called && std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        CHECK(finish_called);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "seek during playback") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        auto source = create_mock_source(44100 * 2); // 2 seconds
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        
        stream.play();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Seek while playing
        CHECK(stream.seek_to_time(std::chrono::milliseconds(1500)));
        // Note: Cannot verify internal position after move
        
        // Should continue playing from new position
        CHECK(stream.is_playing());
        
        stream.stop();
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "multiple callbacks on same stream") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        auto source = create_mock_source(4410); // 0.1 second
        auto stream = device.create_stream(std::move(*source));
        REQUIRE_NOTHROW(stream.open());
        
        std::atomic<int> callback1_count{0};
        std::atomic<int> callback2_count{0};
        
        // Set first callback
        stream.set_finish_callback([&callback1_count](audio_stream& /*s*/) {
            callback1_count++;
        });
        
        stream.play();
        
        // Change callback while playing
        stream.set_finish_callback([&callback2_count](audio_stream& /*s*/) {
            callback2_count++;
        });
        
        // Wait for finish
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Only the second callback should have been called
        CHECK(callback1_count == 0);
        CHECK(callback2_count == 1);
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "stream destruction order stress") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        // Create streams with interdependencies
        std::vector<std::shared_ptr<audio_stream>> streams;
        std::atomic<int> destruction_order{0};
        
        for (int i = 0; i < 5; ++i) {
            auto source = create_mock_source(2205);
            auto stream = std::make_shared<audio_stream>(device.create_stream(std::move(*source)));
            REQUIRE_NOTHROW(stream->open());
            
            if (i > 0) {
                // Create circular references through callbacks
                auto prev_stream = streams[static_cast<size_t>(i-1)];
                stream->set_finish_callback([prev_stream, &destruction_order](audio_stream& /*s*/) {
                    destruction_order++;
                    // Access previous stream
                    if (prev_stream) {
                        prev_stream->is_playing();
                    }
                });
            }
            
            streams.push_back(stream);
            stream->play();
        }
        
        // Clear in various orders
        streams.erase(streams.begin() + 2); // Remove middle
        streams.clear(); // Clear remaining
        
        // Should not crash or hang
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

} // namespace musac::test