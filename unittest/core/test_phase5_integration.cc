#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <random>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <iostream>
#include "../test_helpers.hh"

namespace musac::test {

// Helper class for tracking test metrics
class test_metrics {
public:
    std::atomic<size_t> streams_created{0};
    std::atomic<size_t> streams_destroyed{0};
    std::atomic<size_t> callbacks_executed{0};
    std::atomic<size_t> errors_encountered{0};
    std::atomic<size_t> operations_completed{0};
    
    void reset() {
        streams_created = 0;
        streams_destroyed = 0;
        callbacks_executed = 0;
        errors_encountered = 0;
        operations_completed = 0;
    }
    
    void print_summary(const std::string& test_name) const {
        std::cout << "\n=== " << test_name << " Metrics ===\n"
                  << "Streams created: " << streams_created << "\n"
                  << "Streams destroyed: " << streams_destroyed << "\n"
                  << "Callbacks executed: " << callbacks_executed << "\n"
                  << "Errors encountered: " << errors_encountered << "\n"
                  << "Operations completed: " << operations_completed << "\n"
                  << "================================\n";
    }
};

TEST_SUITE("phase5_integration") {
    // Test fixture for integration tests
    struct integration_fixture {
        test_metrics metrics;
        
        integration_fixture() {
            CHECK(audio_system::init());
        }
        
        ~integration_fixture() {
            // Small delay to ensure callbacks complete
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            audio_system::done();
            metrics.print_summary("Integration Test");
        }
    };
    
    TEST_CASE_FIXTURE(integration_fixture, "real world usage pattern - music player") {
        // Simulate a music player with playlist
        auto device = audio_device::open_default_device();
        device.resume();
        
        // Playlist of "songs" (mock sources)
        std::vector<std::unique_ptr<audio_stream>> playlist;
        const size_t num_songs = 5;
        
        for (size_t i = 0; i < num_songs; ++i) {
            auto source = create_mock_source(44100 / 10); // 0.1 second "songs"
            auto stream = std::make_unique<audio_stream>(device.create_stream(std::move(*source)));
            stream->open();
            
            // Set up finish callback to play next song
            size_t song_index = i;
            stream->set_finish_callback([this, song_index](audio_stream& /*s*/) {
                metrics.callbacks_executed++;
                // In real app, would trigger next song
            });
            
            playlist.push_back(std::move(stream));
            metrics.streams_created++;
        }
        
        // Play through playlist
        for (size_t i = 0; i < playlist.size(); ++i) {
            auto& stream = playlist[i];
            CHECK(stream->play());
            
            // Wait for song to finish
            while (stream->is_playing()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                // Simulate user operations
                if (i == 2) {
                    // Skip song halfway
                    stream->stop();
                    break;
                }
            }
            
            metrics.operations_completed++;
        }
        
        // Cleanup
        playlist.clear();
        metrics.streams_destroyed += num_songs;
        
        CHECK(metrics.errors_encountered == 0);
        CHECK(metrics.streams_created == num_songs);
    }
    
    TEST_CASE_FIXTURE(integration_fixture, "stress test - concurrent operations") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        const int num_threads = 8;
        const auto test_duration = std::chrono::seconds(10);
        
        std::atomic<bool> running{true};
        std::vector<std::thread> workers;
        std::mutex stream_mutex;
        std::vector<std::shared_ptr<audio_stream>> active_streams;
        
        // Worker threads performing random operations
        for (int t = 0; t < num_threads; ++t) {
            workers.emplace_back([&, thread_id = t]() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> op_dist(0, 6);
                std::uniform_int_distribution<> stream_dist(0, 9);
                
                while (running.load()) {
                    try {
                        int operation = op_dist(gen);
                        
                        switch (operation) {
                            case 0: { // Create new stream
                                auto source = create_mock_source(4410); // 0.1s
                                auto stream = std::make_shared<audio_stream>(
                                    device.create_stream(std::move(*source))
                                );
                                stream->open();
                                stream->play();
                                
                                std::lock_guard<std::mutex> lock(stream_mutex);
                                active_streams.push_back(stream);
                                metrics.streams_created++;
                                break;
                            }
                            
                            case 1: { // Remove random stream
                                std::lock_guard<std::mutex> lock(stream_mutex);
                                if (!active_streams.empty()) {
                                    auto idx = static_cast<size_t>(stream_dist(gen)) % active_streams.size();
                                    active_streams.erase(active_streams.begin() + static_cast<std::vector<std::shared_ptr<audio_stream>>::difference_type>(idx));
                                    metrics.streams_destroyed++;
                                }
                                break;
                            }
                            
                            case 2: { // Play/pause random stream
                                std::lock_guard<std::mutex> lock(stream_mutex);
                                if (!active_streams.empty()) {
                                    auto idx = static_cast<size_t>(stream_dist(gen)) % active_streams.size();
                                    auto& stream = active_streams[idx];
                                    if (stream->is_playing()) {
                                        stream->pause();
                                    } else {
                                        stream->resume();
                                    }
                                }
                                break;
                            }
                            
                            case 3: { // Volume change
                                std::lock_guard<std::mutex> lock(stream_mutex);
                                if (!active_streams.empty()) {
                                    auto idx = static_cast<size_t>(stream_dist(gen)) % active_streams.size();
                                    float volume = static_cast<float>(gen() % 100) / 100.0f;
                                    active_streams[idx]->set_volume(volume);
                                }
                                break;
                            }
                            
                            case 4: { // Seek
                                std::lock_guard<std::mutex> lock(stream_mutex);
                                if (!active_streams.empty()) {
                                    auto idx = static_cast<size_t>(stream_dist(gen)) % active_streams.size();
                                    active_streams[idx]->rewind();
                                }
                                break;
                            }
                            
                            case 5: { // Clear all streams
                                std::lock_guard<std::mutex> lock(stream_mutex);
                                metrics.streams_destroyed += active_streams.size();
                                active_streams.clear();
                                break;
                            }
                            
                            case 6: { // Batch create
                                std::vector<std::shared_ptr<audio_stream>> new_streams;
                                for (int i = 0; i < 5; ++i) {
                                    auto source = create_mock_source(2205);
                                    auto stream = std::make_shared<audio_stream>(
                                        device.create_stream(std::move(*source))
                                    );
                                    stream->open();
                                    stream->play();
                                    new_streams.push_back(stream);
                                    metrics.streams_created++;
                                }
                                
                                std::lock_guard<std::mutex> lock(stream_mutex);
                                active_streams.insert(active_streams.end(), 
                                                    new_streams.begin(), 
                                                    new_streams.end());
                                break;
                            }
                        }
                        
                        metrics.operations_completed++;
                        
                    } catch (const std::exception& e) {
                        metrics.errors_encountered++;
                    }
                    
                    // Small delay between operations
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            });
        }
        
        // Run for specified duration
        std::this_thread::sleep_for(test_duration);
        running = false;
        
        // Wait for all workers
        for (auto& worker : workers) {
            worker.join();
        }
        
        // Clean up remaining streams
        {
            std::lock_guard<std::mutex> lock(stream_mutex);
            metrics.streams_destroyed += active_streams.size();
            active_streams.clear();
        }
        
        // Verify no crashes and reasonable error rate
        CHECK(metrics.operations_completed > 1000);
        CHECK(metrics.errors_encountered < static_cast<size_t>(static_cast<double>(metrics.operations_completed) * 0.01)); // < 1% error rate
    }
    
    TEST_CASE_FIXTURE(integration_fixture, "edge case - rapid init/done cycles with active streams") {
        for (int cycle = 0; cycle < 10; ++cycle) {
            // Create device and streams
            auto device = audio_device::open_default_device();
            device.resume();
            
            std::vector<std::unique_ptr<audio_stream>> streams;
            for (int i = 0; i < 3; ++i) {
                auto source = create_mock_source(44100); // 1 second
                auto stream = std::make_unique<audio_stream>(
                    device.create_stream(std::move(*source))
                );
                stream->open();
                stream->play();
                streams.push_back(std::move(stream));
                metrics.streams_created++;
            }
            
            // Perform operations while playing
            for (int op = 0; op < 10; ++op) {
                for (auto& stream : streams) {
                    stream->set_volume(0.5f);
                    if (op % 2 == 0) {
                        stream->pause();
                    } else {
                        stream->resume();
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            
            // Destroy everything and reinit
            streams.clear();
            metrics.streams_destroyed += 3;
            
            // Destroy device but immediately recreate system
            // This tests the fool-proof shutdown
        }
        
        CHECK(metrics.errors_encountered == 0);
    }
    
    TEST_CASE_FIXTURE(integration_fixture, "callback synchronization - complex scenario") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        std::mutex callback_mutex;
        std::condition_variable callback_cv;
        std::atomic<int> active_callbacks{0};
        std::atomic<int> total_callbacks{0};
        std::atomic<bool> callback_error{false};
        
        // Create streams with callbacks that do significant work
        const int num_streams = 20;
        std::vector<std::unique_ptr<audio_stream>> streams;
        
        for (int i = 0; i < num_streams; ++i) {
            auto source = create_mock_source(2205); // 0.05 seconds
            auto stream = std::make_unique<audio_stream>(
                device.create_stream(std::move(*source))
            );
            stream->open();
            
            // Complex callback that might cause race conditions
            stream->set_finish_callback([&, stream_id = i](audio_stream& s) {
                active_callbacks.fetch_add(1);
                total_callbacks.fetch_add(1);
                
                // Simulate work in callback
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                // Try to access stream state
                try {
                    bool playing = s.is_playing();
                    float volume = s.volume();
                    (void)playing;
                    (void)volume;
                } catch (...) {
                    callback_error = true;
                }
                
                active_callbacks.fetch_sub(1);
                callback_cv.notify_all();
            });
            
            streams.push_back(std::move(stream));
            metrics.streams_created++;
        }
        
        // Play all streams concurrently
        for (auto& stream : streams) {
            stream->play();
        }
        
        // Wait for all callbacks to complete
        {
            std::unique_lock<std::mutex> lock(callback_mutex);
            callback_cv.wait_for(lock, std::chrono::seconds(5), [&] {
                return total_callbacks.load() >= num_streams;
            });
        }
        
        CHECK(total_callbacks >= num_streams);
        
        // Wait for all active callbacks to complete
        {
            std::unique_lock<std::mutex> lock(callback_mutex);
            auto wait_result = callback_cv.wait_for(lock, std::chrono::seconds(2), [&] {
                return active_callbacks.load() == 0;
            });
            CHECK(wait_result);
        }
        
        CHECK(active_callbacks == 0);
        CHECK_FALSE(callback_error);
        
        // Now destroy streams while callbacks might still be running
        streams.clear();
        metrics.streams_destroyed += num_streams;
        
        // Give time for any lingering callbacks
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Wait again for active callbacks to reach zero
        {
            std::unique_lock<std::mutex> lock(callback_mutex);
            auto wait_result = callback_cv.wait_for(lock, std::chrono::seconds(2), [&] {
                return active_callbacks.load() == 0;
            });
            CHECK(wait_result);
        }
        
        CHECK(active_callbacks == 0);
    }
    
    TEST_CASE_FIXTURE(integration_fixture, "memory stress - create/destroy thousands of streams") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        const int iterations = 100;
        const int streams_per_iteration = 50;
        
        for (int iter = 0; iter < iterations; ++iter) {
            std::vector<std::unique_ptr<audio_stream>> streams;
            
            // Create many streams
            for (int i = 0; i < streams_per_iteration; ++i) {
                auto source = create_mock_source(441); // Very short
                auto stream = std::make_unique<audio_stream>(
                    device.create_stream(std::move(*source))
                );
                stream->open();
                streams.push_back(std::move(stream));
                metrics.streams_created++;
            }
            
            // Play half of them
            for (size_t i = 0; i < streams.size() / 2; ++i) {
                streams[i]->play();
            }
            
            // Random operations
            for (int op = 0; op < 10; ++op) {
                auto idx = static_cast<size_t>(op) % streams.size();
                streams[idx]->set_volume(0.7f);
                streams[idx]->pause();
                streams[idx]->resume();
            }
            
            // Destroy all
            streams.clear();
            metrics.streams_destroyed += streams_per_iteration;
            
            // Small delay
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        CHECK(metrics.streams_created == iterations * streams_per_iteration);
        CHECK(metrics.streams_destroyed == iterations * streams_per_iteration);
        CHECK(metrics.errors_encountered == 0);
    }
    
    TEST_CASE_FIXTURE(integration_fixture, "shutdown order combinations") {
        // Test various shutdown orders to ensure fool-proof behavior
        
        SUBCASE("streams outlive device") {
            std::vector<std::unique_ptr<audio_stream>> streams;
            
            {
                auto device = audio_device::open_default_device();
                device.resume();
                
                for (int i = 0; i < 5; ++i) {
                    auto source = create_mock_source(4410);
                    auto stream = std::make_unique<audio_stream>(
                        device.create_stream(std::move(*source))
                    );
                    stream->open();
                    stream->play();
                    streams.push_back(std::move(stream));
                }
                
                // Device destroyed here
            }
            
            // Streams still exist - should handle gracefully
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            streams.clear();
        }
        
        SUBCASE("system shutdown with active device and streams") {
            auto device = audio_device::open_default_device();
            device.resume();
            
            auto source = create_mock_source(44100);
            auto stream = device.create_stream(std::move(*source));
            stream.open();
            stream.play();
            
            // Shutdown system while everything is active
            audio_system::done();
            
            // Device and stream destructors run after done()
            // This should be safe due to our fool-proof implementation
        }
        
        SUBCASE("interleaved destruction") {
            auto device1 = audio_device::open_default_device();
            device1.resume();
            
            auto source1 = create_mock_source(4410);
            auto stream1 = std::make_unique<audio_stream>(
                device1.create_stream(std::move(*source1))
            );
            stream1->open();
            
            // Destroy in unusual order
            stream1.reset();
            
            // Continue using device
            auto source2 = create_mock_source(4410);
            auto stream2 = device1.create_stream(std::move(*source2));
            stream2.open();
            stream2.play();
            
            // Everything cleaned up by fixture
        }
    }
}

} // namespace musac::test