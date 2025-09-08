/**
 * @example 04_pc_speaker.cpp
 * @brief PC speaker emulation and MML playback
 * 
 * Demonstrates PC speaker emulation for retro game sounds and music
 * using MML (Music Macro Language).
 */

#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/pc_speaker_stream.hh>
#include "example_common.hh"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

// Classic game sound effects
void play_powerup(musac::pc_speaker_stream& pc) {
    std::cout << "Power-up sound effect\n";
    for (float freq = 200; freq <= 2000; freq *= 1.15f) {
        pc.sound(freq, 20ms);
    }
}

void play_laser(musac::pc_speaker_stream& pc) {
    std::cout << "Laser sound effect\n";
    for (float freq = 2000; freq >= 200; freq /= 1.05f) {
        pc.sound(freq, 10ms);
    }
}

void play_explosion(musac::pc_speaker_stream& pc) {
    std::cout << "Explosion sound effect\n";
    // Random noise-like effect
    for (int i = 0; i < 20; ++i) {
        float freq = 50 + (rand() % 200);
        pc.sound(freq, 20ms);
    }
}

void play_alarm(musac::pc_speaker_stream& pc) {
    std::cout << "Alarm sound\n";
    for (int i = 0; i < 5; ++i) {
        pc.sound(800.0f, 200ms);
        pc.sound(600.0f, 200ms);
    }
}

int main() {
    try {
        // Initialize audio system with default backend
        auto backend = musac::examples::create_default_backend();
        if (!musac::audio_system::init(backend)) {
            std::cerr << "Failed to initialize audio system\n";
            return 1;
        }
        std::cout << "Using " << musac::examples::get_backend_name() << " backend\n";
        
        // Create audio device
        auto device = musac::audio_device::open_default_device(backend);
        
        // Create PC speaker stream  
        auto pc_speaker = device.create_pc_speaker_stream();
        pc_speaker.play();
        
        std::cout << "=== PC Speaker Demo ===\n\n";
        
        // 1. Simple beeps
        std::cout << "1. Simple beeps\n";
        pc_speaker.beep();  // Default 1kHz beep
        std::this_thread::sleep_for(500ms);
        
        pc_speaker.beep(440.0f);  // A4 note
        std::this_thread::sleep_for(500ms);
        
        // 2. Manual tone sequence (arpeggio)
        std::cout << "\n2. C Major Arpeggio\n";
        pc_speaker.sound(261.63f, 200ms);  // C4
        pc_speaker.sound(329.63f, 200ms);  // E4
        pc_speaker.sound(392.00f, 200ms);  // G4
        pc_speaker.sound(523.25f, 400ms);  // C5
        std::this_thread::sleep_for(1s);
        
        // 3. Sound effects
        std::cout << "\n3. Game sound effects\n";
        play_powerup(pc_speaker);
        std::this_thread::sleep_for(1s);
        
        play_laser(pc_speaker);
        std::this_thread::sleep_for(1s);
        
        play_explosion(pc_speaker);
        std::this_thread::sleep_for(1s);
        
        play_alarm(pc_speaker);
        std::this_thread::sleep_for(3s);
        
        // 4. MML music
        std::cout << "\n4. MML Music Examples\n";
        
        // Simple scale
        std::cout << "   - C Major Scale\n";
        pc_speaker.play_mml("T120 L4 C D E F G A B >C");
        std::this_thread::sleep_for(3s);
        
        // Mary Had a Little Lamb
        std::cout << "   - Mary Had a Little Lamb\n";
        pc_speaker.play_mml(
            "T120 L4 "
            "E D C D E E E2 "
            "D D D2 E G G2 "
            "E D C D E E E E "
            "D D E D C"
        );
        std::this_thread::sleep_for(8s);
        
        // Pac-Man death sound
        std::cout << "   - Pac-Man Death Sound\n";
        pc_speaker.play_mml("T200 O3 B >F B F <B >F B F");
        std::this_thread::sleep_for(2s);
        
        // Fanfare
        std::cout << "   - Victory Fanfare\n";
        pc_speaker.play_mml(
            "T140 V12 MS "  // Tempo 140, Volume 12, Staccato
            "L8 O4 "
            "C E G >C< G E "  // Up arpeggio
            "G B >D G D <B "  // Another arpeggio
            "L4 >C C C L2 C"  // Ending
        );
        std::this_thread::sleep_for(4s);
        
        // Complex example with dynamics
        std::cout << "   - Dynamic Example\n";
        pc_speaker.play_mml(
            "T160 "
            "V8 L16 C D E F "    // Quiet, fast notes
            "V12 L8 G G "        // Louder, slower
            "V15 L4 >C"          // Loudest, long note
        );
        std::this_thread::sleep_for(3s);
        
        std::cout << "\nDemo complete!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}