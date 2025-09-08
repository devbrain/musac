#include <iostream>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/test_data/loader.hh>
#include <musac/stream.hh>
#include <musac/sdk/audio_backend.hh>
#include "example_common.hh"
#include <musac/codecs/decoder_mml.hh>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    using namespace musac;
    using namespace std::chrono_literals;
    
    // Initialize test data loader
    test_data::loader::init();
    
    try {
        // Create backend using common helper
        auto backend = musac::examples::create_default_backend();
        std::cout << "Using " << musac::examples::get_backend_name() << " backend for audio output\n";
        
        // Initialize audio system
        audio_system::init(backend);
        
        // Open default device
        auto device = audio_device::open_default_device(backend);
        std::cout << "Audio device opened\n";
        
        // Test both MML files from test_data
        std::cout << "\n=== Testing Bourrée MML ===\n";
        auto source1 = test_data::loader::load(test_data::music_type::mml_bouree);
        auto stream1 = device.create_stream(std::move(source1));
        
        // Helper function to play an MML stream
        auto play_mml_stream = [&device](auto& stream, const std::string& name) {
            // Open stream first (this initializes the decoder)
            try {
                stream.open();
            } catch (const std::exception& e) {
                std::cerr << "Failed to open stream: " << e.what() << "\n";
                return false;
            }
            
            // Get duration (now that decoder is initialized)
            auto duration = stream.duration();
            auto duration_seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
            std::cout << name << " duration: " << duration_seconds << " seconds\n";
            
            // Set up completion callback
            bool playback_done = false;
            stream.set_finish_callback([&playback_done, &name](auto& s) {
                std::cout << name << " playback complete!\n";
                playback_done = true;
            });
            
            // Play the MML
            std::cout << "Playing " << name << "...\n";
            stream.play();
            
            // Wait for playback to complete via callback
            while (!playback_done) {
                std::this_thread::sleep_for(100ms);
            }
            
            return true;
        };
        
        // Set gain and resume device
        device.set_gain(1.0f);
        device.resume();
        
        // Play Bourrée
        if (!play_mml_stream(stream1, "Bourrée")) {
            return 1;
        }
        
        // Load and play complex MML
        std::cout << "\n=== Testing Complex MML ===\n";
        auto source2 = test_data::loader::load(test_data::music_type::mml_complex);
        auto stream2 = device.create_stream(std::move(source2));
        
        if (!play_mml_stream(stream2, "Complex MML")) {
            return 1;
        }
        
        // Cleanup
        test_data::loader::done();
        audio_system::done();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}