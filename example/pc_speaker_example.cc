#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/pc_speaker_stream.hh>
#include <musac/sdk/audio_backend.hh>
#ifdef MUSAC_HAS_SDL3_BACKEND
#include <musac_backends/sdl3/sdl3_backend.hh>
#elif MUSAC_HAS_SDL2_BACKEND
#include <musac_backends/sdl2/sdl2_backend.hh>
#endif
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <mutex>

using namespace std::chrono_literals;

void play_scale(musac::pc_speaker_stream& speaker) {
    // C major scale frequencies
    speaker.sound(261.63f, 200ms);  // C4
    speaker.sound(293.66f, 200ms);  // D4
    speaker.sound(329.63f, 200ms);  // E4
    speaker.sound(349.23f, 200ms);  // F4
    speaker.sound(392.00f, 200ms);  // G4
    speaker.sound(440.00f, 200ms);  // A4
    speaker.sound(493.88f, 200ms);  // B4
    speaker.sound(523.25f, 200ms);  // C5
}

void play_mario_coin(musac::pc_speaker_stream& speaker) {
    // Simplified Mario coin sound
    speaker.sound(988.0f, 80ms);   // B5
    speaker.sound(1319.0f, 400ms); // E6
}

void play_alarm(musac::pc_speaker_stream& speaker) {
    // Alarm sound
    for (int i = 0; i < 3; i++) {
        speaker.sound(800.0f, 100ms);
        speaker.silence(50ms);
        speaker.sound(600.0f, 100ms);
        speaker.silence(50ms);
    }
}

void play_mml_examples(musac::pc_speaker_stream& speaker, 
                       std::atomic<bool>& demo_complete,
                       std::condition_variable& cv,
                       std::mutex& cv_mutex) {
    std::cout << "\nMML (Music Macro Language) Examples\n";
    std::cout << "====================================\n\n";
    
    // Helper to wait for sound completion
    auto wait_for_completion = [&](int expected_ms) {
        std::unique_lock<std::mutex> lock(cv_mutex);
        demo_complete = false;
        speaker.set_finish_callback([&](auto&) {
            std::unique_lock<std::mutex> lock(cv_mutex);
            demo_complete = true;
            cv.notify_one();
        });
        speaker.play();
        cv.wait_for(lock, std::chrono::milliseconds(expected_ms + 500), 
                    [&]{ return demo_complete.load(); });
        speaker.stop();
    };
    
    // Example 1: Simple scale
    std::cout << "6. C Major Scale (MML)\n";
    if (!speaker.play_mml("T120 L4 C D E F G A B >C")) {
        std::cerr << "Failed to parse MML\n";
        for (const auto& warning : speaker.get_mml_warnings()) {
            std::cerr << "  Warning: " << warning << "\n";
        }
        return;
    }
    wait_for_completion(3000);
    
    // Example 2: Mary Had a Little Lamb
    std::cout << "7. Mary Had a Little Lamb (MML)\n";
    speaker.play_mml("T120 L4 E D C D E E E2 D D D2 E G G2");
    wait_for_completion(3000);
    
    // Example 3: Twinkle Twinkle Little Star
    std::cout << "8. Twinkle Twinkle Little Star (MML)\n";
    speaker.play_mml("T100 L4 C C G G A A G2 F F E E D D C2");
    wait_for_completion(3000);
    
    // Example 4: Ode to Joy
    std::cout << "9. Ode to Joy (MML)\n";
    speaker.play_mml("O3 T120 L8 E E F G G F E D C C D E E. D16 D4");
    wait_for_completion(3000);
    
    // Example 5: Using octave changes
    std::cout << "10. Octave Changes Demo (MML)\n";
    speaker.play_mml("T180 L8 O3 C O4 C O5 C O6 C O5 C O4 C O3 C");
    wait_for_completion(2000);
    
    // Example 6: Staccato vs Legato
    std::cout << "11. Articulation Demo (MML)\n";
    speaker.play_mml("T120 L4 MS C D E F ML C D E F MN C D E F");
    wait_for_completion(3000);
    
    // Example 7: Complex rhythm with dots
    std::cout << "12. Dotted Notes Demo (MML)\n";
    speaker.play_mml("T120 L4 C. D8 E2 F8 F8 F8 F8 G2.");
    wait_for_completion(3000);
    
    // Example 8: Error handling demo
    std::cout << "13. MML Error Handling Demo\n";
    bool success = speaker.play_mml("T120 L4 C D E Z G A B", false); // Z is invalid note
    if (!success) {
        std::cout << "  Parsing failed (as expected)\n";
    }
    auto warnings = speaker.get_mml_warnings();
    if (!warnings.empty()) {
        std::cout << "  Warnings/Errors:\n";
        for (const auto& warning : warnings) {
            std::cout << "    - " << warning << "\n";
        }
    }
}

int main() {
    try {
        // Create backend
        std::shared_ptr<musac::audio_backend> backend;
        
#ifdef MUSAC_HAS_SDL3_BACKEND
        backend = musac::create_sdl3_backend();
        std::cout << "Using SDL3 backend for audio output\n";
#elif MUSAC_HAS_SDL2_BACKEND
        backend = musac::create_sdl2_backend();
        std::cout << "Using SDL2 backend for audio output\n";
#else
        std::cerr << "No audio backend available\n";
        return 1;
#endif
        
        // Initialize audio system
        if (!musac::audio_system::init(backend)) {
            std::cerr << "Failed to initialize audio system\n";
            return 1;
        }
        
        // Open default audio device
        auto device = musac::audio_device::open_default_device(backend);
        std::cout << "Audio device opened\n";
        
        // Set gain to ensure audio is audible
        device.set_gain(1.0f);
        
        // Resume the device to start audio processing
        device.resume();
        
        // Create PC speaker stream
        musac::pc_speaker_stream speaker = device.create_pc_speaker_stream();
        std::cout << "PC speaker stream created\n";
        
        std::cout << "PC Speaker Example\n";
        std::cout << "==================\n\n";
        
        // Open the stream first
        speaker.open();
        
        // Setup for completion callbacks
        std::atomic<bool> demo_complete{false};
        std::condition_variable cv;
        std::mutex cv_mutex;
        
        // Helper lambda to wait for completion
        auto wait_for_demo = [&](int expected_ms) {
            std::unique_lock<std::mutex> lock(cv_mutex);
            demo_complete = false;
            speaker.set_finish_callback([&](auto&) {
                std::unique_lock<std::mutex> callback_lock(cv_mutex);
                demo_complete = true;
                cv.notify_one();
            });
            speaker.play();
            cv.wait_for(lock, std::chrono::milliseconds(expected_ms + 500), 
                        [&]{ return demo_complete.load(); });
            speaker.stop();
        };
        
        // Play a simple beep
        std::cout << "1. Simple beep (1000Hz for 100ms)\n";
        speaker.beep();  // Queue the beep first
        wait_for_demo(500);
        
        // Play a scale
        std::cout << "2. C Major Scale\n";
        play_scale(speaker);
        wait_for_demo(2000);
        
        // Play Mario coin sound
        std::cout << "3. Mario Coin Sound\n";
        play_mario_coin(speaker);
        wait_for_demo(1000);
        
        // Play alarm
        std::cout << "4. Alarm Sound\n";
        play_alarm(speaker);
        wait_for_demo(2000);
        
        // Play multiple tones in sequence
        std::cout << "5. Random Melody\n";
        speaker.sound(440.0f, 200ms);   // A4
        speaker.sound(494.0f, 150ms);   // B4
        speaker.sound(523.0f, 150ms);   // C5
        speaker.sound(587.0f, 200ms);   // D5
        speaker.sound(523.0f, 150ms);   // C5
        speaker.sound(494.0f, 150ms);   // B4
        speaker.sound(440.0f, 400ms);   // A4
        wait_for_demo(2000);
        
        // Play MML examples
        play_mml_examples(speaker, demo_complete, cv, cv_mutex);
        
        std::cout << "\nAll demos completed. Waiting 2 seconds before exit...\n";
        std::this_thread::sleep_for(2s);
        
        // Cleanup
        musac::audio_system::done();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}