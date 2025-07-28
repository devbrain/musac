//
// Created by igor on 3/17/25.
//

#include <iostream>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include "loader.hh"
#include "musac/audio_loader.hh"



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

    audio_system::init();
    auto drivers = audio_hardware::enumerate(true);
    for (const auto& d : drivers) {
        std::cout << d.get_device_id() << "[" << d.get_name() << "]" << " Format 0x" << std::hex << d.get_format() <<
            std::dec
            << " Channels " << d.get_channels() << " Freq " << d.get_freq();
        if (d.is_default()) {
            std::cout << " *";
        }
        std::cout << std::endl;
    } {
        auto d = audio_hardware::get_default_device(true);
        auto device = d.open();
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


        auto strm = device.create_stream(loader::load(music_type::mid));
        audio_stream stream(std::move(strm));

        device.resume();

        bool done = false;
        stream.set_finish_callback([&done](auto& s) {
            done = true;
        });
        stream.open();
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
            SDL_Delay(500);
        }
    }

    audio_system::done();
    loader::done();
}
