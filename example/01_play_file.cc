/**
 * @example 01_play_file.cpp
 * @brief Basic example: Playing an audio file
 * 
 * This example demonstrates the simplest way to play an audio file
 * using the musac library. It loads a file and plays it to completion.
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
        std::cerr << "Supported formats: WAV, MP3, FLAC, OGG, AIFF, MOD, etc.\n";
        return 1;
    }
    
    try {
        // Step 1: Initialize audio system with configured backend
        auto backend = musac::examples::create_default_backend();
        auto registry = musac::create_registry_with_all_codecs();
        if (!musac::audio_system::init(backend, registry)) {
            std::cerr << "Failed to initialize audio system\n";
            return 1;
        }
        std::cout << "Using " << musac::examples::get_backend_name() << " backend\n";
        
        // Step 2: Create and open the default audio device
        auto device = musac::audio_device::open_default_device(backend);
        std::cout << "Audio device opened successfully\n";
        
        // Step 3: Load the audio file
        auto io = musac::io_from_file(argv[1], "rb");
        musac::audio_source source(std::move(io), registry.get());
        std::cout << "Loaded file: " << argv[1] << '\n';
        
        // Step 4: Create a stream from the source
        auto stream = device.create_stream(std::move(source));
        
        // Step 5: Start playback
        stream.play();
        std::cout << "Playing... Press Ctrl+C to stop\n";
        
        // Step 6: Wait while playing
        // Check every 100ms if still playing
        while (stream.is_playing()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "Playback finished\n";
        
        // Cleanup
        musac::audio_system::done();
        
    } catch (const musac::device_error& e) {
        std::cerr << "Device error: " << e.what() << '\n';
        return 1;
    } catch (const musac::decoder_error& e) {
        std::cerr << "Decoder error: " << e.what() << '\n';
        std::cerr << "File format might not be supported\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}