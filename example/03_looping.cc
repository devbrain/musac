/**
 * @example 03_looping.cpp
 * @brief Simple looping playback example
 * 
 * Demonstrates basic audio playback with manual looping.
 * Note: Advanced loop features may not be available in all configurations.
 */


#include <musac/audio_device.hh>
#include <musac/audio_source.hh>
#include <musac/audio_system.hh>
#include <musac/error.hh>
#include <musac/sdk/io_stream.hh>
#include <musac/codecs/register_codecs.hh>
#include <musac/stream.hh>
#include "example_common.hh"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <audio_file> [play_count]\n";
        std::cerr << "  play_count: number of times to play (default = 1)\n";
        return 1;
    }
    
    try {
        // Initialize audio system
        auto backend = musac::examples::create_default_backend();
        auto registry = musac::create_registry_with_all_codecs();
        if (!musac::audio_system::init(backend, registry)) {
            std::cerr << "Failed to initialize audio system\n";
            return 1;
        }
        
        auto device = musac::audio_device::open_default_device(backend);
        
        auto io = musac::io_from_file(argv[1], "rb");
        musac::audio_source source(std::move(io), registry.get());
        auto stream = device.create_stream(std::move(source));
        
        // Parse play count from command line
        int play_count = (argc >= 3) ? std::atoi(argv[2]) : 1;
        
        // Simple manual looping
        for (int i = 1; i <= play_count || play_count == 0; ++i) {
            if (play_count == 0) {
                std::cout << "Playing loop #" << i << " (infinite mode, press Ctrl+C to stop)\n";
            } else {
                std::cout << "Playing " << i << " of " << play_count << "\n";
            }
            
            // Create a new stream for each loop (simple approach)
            auto io = musac::io_from_file(argv[1], "rb");
            musac::audio_source source(std::move(io), registry.get());
            auto stream = device.create_stream(std::move(source));
            
            // Start playback
            stream.play();
            
            // Wait for playback to complete
            while (stream.is_playing()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            // Small pause between loops
            if (i < play_count || play_count == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        
        std::cout << "Playback completed\n";
        
        // Cleanup
        musac::audio_system::done();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}