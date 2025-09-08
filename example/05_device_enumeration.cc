/**
 * @example 05_device_enumeration.cpp
 * @brief Audio device enumeration and selection
 * 
 * Shows how to list available audio devices and select a specific one
 * for playback.
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
        std::cout << "Using " << musac::examples::get_backend_name() << " backend\n\n";
        
        // Step 1: Enumerate all available audio devices
        std::cout << "=== Available Audio Devices ===\n";
        auto devices = musac::audio_device::enumerate_devices(backend);
        
        if (devices.empty()) {
            std::cerr << "No audio devices found!\n";
            return 1;
        }
        
        // Display device information
        for (size_t i = 0; i < devices.size(); ++i) {
            const auto& info = devices[i];
            std::cout << i << ": " << info.name;
            
            // Show device capabilities
            std::cout << " (";
            if (info.channels == 1) {
                std::cout << "Mono";
            } else if (info.channels == 2) {
                std::cout << "Stereo";
            } else {
                std::cout << info.channels << " channels";
            }
            std::cout << ", " << info.sample_rate << " Hz";
            
            if (info.is_default) {
                std::cout << ", DEFAULT";
            }
            std::cout << ")\n";
        }
        
        // Step 2: Let user choose a device
        std::cout << "\nSelect device number (or press Enter for default): ";
        std::string input;
        std::getline(std::cin, input);
        
        // Open the selected device
        musac::audio_device device = [&]() {
            if (input.empty()) {
                // Use default device
                std::cout << "Using default device\n";
                return musac::audio_device::open_default_device(backend);
            } else {
                // Use selected device
                int selection = std::stoi(input);
                if (selection < 0 || selection >= static_cast<int>(devices.size())) {
                    throw std::runtime_error("Invalid device number");
                }
                
                std::cout << "Opening device: " << devices[selection].name << '\n';
                return musac::audio_device::open_device(backend, devices[selection].id);
            }
        }();
        
        // Step 3: Play audio on selected device
        std::cout << "Loading file: " << argv[1] << '\n';
        auto io = musac::io_from_file(argv[1], "rb");
        musac::audio_source source(std::move(io), registry.get());
        auto stream = device.create_stream(std::move(source));
        
        stream.play();
        std::cout << "Playing on selected device...\n";
        
        // Display current device info
        std::cout << "Current device: " << device.get_device_name() << '\n';
        std::cout << "Format: " << device.get_channels() << " channels, "
                  << device.get_freq() << " Hz\n";
        
        // Wait for playback to complete
        while (stream.is_playing()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "Playback complete\n";
        
        // Cleanup
        musac::audio_system::done();
        
    } catch (const musac::device_error& e) {
        std::cerr << "Device error: " << e.what() << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}