/**
 * @example 02_volume_control.cpp
 * @brief Volume and stereo positioning control
 * 
 * This example shows how to control volume and stereo positioning
 * of audio streams during playback.
 */

#include "example_common.hh"
#include <musac/audio_device.hh>
#include <musac/audio_source.hh>
#include <musac/audio_system.hh>
#include <musac/stream.hh>
#include <musac/error.hh>
#include <musac/sdk/io_stream.hh>
#include <musac/codecs/register_codecs.hh>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <audio_file>\n";
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
        
        // Initialize audio
        auto device = musac::audio_device::open_default_device(backend);
        
        auto io = musac::io_from_file(argv[1], "rb");
        musac::audio_source source(std::move(io), registry.get());
        auto stream = device.create_stream(std::move(source));
        
        // Start playback at 50% volume
        stream.set_volume(0.5f);
        stream.play();
        
        std::cout << "Playing at 50% volume\n";
        std::cout << "Controls:\n";
        std::cout << "  + : Increase volume\n";
        std::cout << "  - : Decrease volume\n";
        std::cout << "  l : Pan left\n";
        std::cout << "  r : Pan right\n";
        std::cout << "  c : Center\n";
        std::cout << "  q : Quit\n\n";
        
        float volume = 0.5f;
        float pan = 0.0f;
        
        // Interactive control loop
        while (stream.is_playing()) {
            if (std::cin.peek() != EOF) {
                char cmd;
                std::cin >> cmd;
                
                switch (cmd) {
                    case '+':
                        volume = std::min(volume + 0.1f, 2.0f);
                        stream.set_volume(volume);
                        std::cout << "Volume: " << (int)(volume * 100) << "%\n";
                        break;
                        
                    case '-':
                        volume = std::max(volume - 0.1f, 0.0f);
                        stream.set_volume(volume);
                        std::cout << "Volume: " << (int)(volume * 100) << "%\n";
                        break;
                        
                    case 'l':
                        pan = std::max(pan - 0.25f, -1.0f);
                        stream.set_stereo_position(pan);
                        std::cout << "Pan: " << (pan < 0 ? "Left" : pan > 0 ? "Right" : "Center") 
                                  << " (" << pan << ")\n";
                        break;
                        
                    case 'r':
                        pan = std::min(pan + 0.25f, 1.0f);
                        stream.set_stereo_position(pan);
                        std::cout << "Pan: " << (pan < 0 ? "Left" : pan > 0 ? "Right" : "Center")
                                  << " (" << pan << ")\n";
                        break;
                        
                    case 'c':
                        pan = 0.0f;
                        stream.set_stereo_position(pan);
                        std::cout << "Pan: Center\n";
                        break;
                        
                    case 'q':
                        stream.stop();
                        std::cout << "Stopping...\n";
                        break;
                        
                    default:
                        std::cout << "Unknown command: " << cmd << '\n';
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Cleanup
        musac::audio_system::done();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}