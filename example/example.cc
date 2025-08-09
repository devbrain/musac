//
// Created by igor on 3/17/25.
//

#include <iostream>
#include <thread>
#include <chrono>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/test_data/loader.hh>
#include "musac/audio_loader.hh"
#include <musac/stream.hh>
#include <musac/sdk/audio_backend_v2.hh>
#include <musac/backends/sdl3/sdl3_backend.hh>
#include <musac/backends/null/null_backend.hh>



#define PPCAT_NX(A, B) A ## B

/*
 * Concatenate preprocessor tokens A and B after macro-expanding them.
 */
#define PPCAT(A, B) PPCAT_NX(A, B)

#define DN(N) PPCAT(PPCAT(EFILE, N), _BRX)
#define DS(N) PPCAT(DN(N), _size)
#define STR1(x) #x
#define STR(x) STR1(x)
#define LOAD(N) load(STR(DN(N)), DN(N), DS(N))


int main(int argc, char* argv[]) {
    using namespace musac;

    // Create backend explicitly using v2 API
    std::shared_ptr<audio_backend_v2> backend;
    
#ifdef MUSAC_HAS_SDL3_BACKEND
    // Use SDL3 backend for actual audio output
    backend = create_sdl3_backend_v2();
    std::cout << "Using SDL3 backend for audio output" << std::endl;
#else
    // Use Null backend for testing (no sound)
    backend = create_null_backend_v2();
    std::cout << "Using Null backend - no sound will be produced" << std::endl;
#endif

    // Initialize audio system with explicit backend
    audio_system::init(backend);
    // Enumerate devices using the backend
    auto devices = audio_device::enumerate_devices(backend, true);
    for (const auto& d : devices) {
        std::cout << "[" << d.name << "]" << " ID: " << d.id
            << " Channels " << d.channels << " Freq " << d.sample_rate
            << (d.is_default ? " (Default)" : "");
        std::cout << std::endl;
    } {
        // Open default device using the backend
        auto device = audio_device::open_default_device(backend);
        //auto strm = device.create_stream(loader::load(music_type::mp3));
        // 1 - on sunk
        // 2 - applause
        // 3 - menu select
        // 4 - on move
        // 5 - end of scores
        // 6 - explosion
        // 7 - laugh
        // 8 - score counting
        // 10 - end intro
        // 11 - portal transition
        // 12 - tick


        auto strm = device.create_stream(musac::test_data::loader::load(musac::test_data::music_type::vorbis));
        audio_stream stream(std::move(strm));

        // Set gain to ensure audio is audible
        device.set_gain(2.0f);
        device.resume();

        bool done = false;
        stream.set_finish_callback([&done](auto& s) {
            std::cout << "Stream finished!" << std::endl;
            done = true;
        });
        try {
            stream.open();
        } catch (const std::exception& e) {
            std::cerr << "Failed to open stream: " << e.what() << std::endl;
            return 1;
        }
        stream.play();
        float s = 0;
        bool in_stop = false;
        while (!done) {
            std::cout << s << std::endl;
            s = s + 0.5f;
            if (s > 10) {
                if (!in_stop) {
                    stream.stop(std::chrono::seconds{2});
                    in_stop = true;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    audio_system::done();
    musac::test_data::loader::done();
}
