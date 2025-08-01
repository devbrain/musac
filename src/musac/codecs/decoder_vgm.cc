//
// Created by igor on 3/20/25.
//

#include <iostream>
#include <chrono>
#include <musac/codecs/decoder_vgm.hh>
#include "musac/codecs/vgm/vgm_player.hh"

#define SAMPLE_RATE 44100

namespace musac {
    struct decoder_vgm::impl {
        impl() {
        }

        vgm_player m_player;
    };


    decoder_vgm::decoder_vgm()
        : m_pimpl(std::make_unique <impl>()) {
    }

    decoder_vgm::~decoder_vgm() = default;

    bool decoder_vgm::open(SDL_IOStream* rwops) {
        bool result = m_pimpl->m_player.load(rwops);
        if (result) {
            set_is_open(true);
        }
        return result;
    }

    unsigned int decoder_vgm::get_channels() const {
        return 2;
    }

    unsigned int decoder_vgm::get_rate() const {
        return SAMPLE_RATE;
    }

    bool decoder_vgm::rewind() {
        return false;
    }

    std::chrono::microseconds decoder_vgm::duration() const {
        return {};
    }

    bool decoder_vgm::seek_to_time([[maybe_unused]] std::chrono::microseconds pos) {
        return false;
    }

    unsigned int decoder_vgm::do_decode(float buf[], unsigned int len, bool& callAgain) {
        int rc = m_pimpl->m_player.render(buf, len);
        if (m_pimpl->m_player.done() && rc == 0) {
            return 0;
        }

        if ((unsigned int) rc < len) {
            callAgain = true;
        }
        return (unsigned int)rc;
    }
}
