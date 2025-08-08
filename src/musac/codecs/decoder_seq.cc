//
// Created by igor on 3/23/25.
//

#include <musac/codecs/decoder_seq.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include "musac/codecs/seq/player.h"
#include "musac/codecs/seq/midi_opl.h"
#include <memory>

namespace musac {
    struct decoder_seq::impl {
        impl() : m_player(std::make_unique<ymfmidi::OPLPlayer>()) {
            m_player->loadPatches(GENMIDI_wopl, GENMIDI_wopl_size);
            m_player->setLoop(false);
            unsigned int sampleRate = 44100;
            double gain = 1.0;
            double filter = 5.0;
	        m_player->setSampleRate(sampleRate);
	        m_player->setGain(gain);
	        m_player->setFilter(filter);
	        m_player->setStereo(true);
        }
        ~impl() = default;
        std::unique_ptr<ymfmidi::OPLPlayer> m_player;
    };

    decoder_seq::decoder_seq()
        : m_pimpl(std::make_unique<impl>()){
    }

    decoder_seq::~decoder_seq() = default;

    void decoder_seq::open(io_stream* rwops) {
        bool result = m_pimpl->m_player->loadSequence(rwops);
        if (!result) {
            THROW_RUNTIME("Failed to load SEQ file");
        }
        set_is_open(true);
    }

    channels_t decoder_seq::get_channels() const {
        return m_pimpl->m_player->stereo() ? 2 : 1;
    }

    sample_rate_t decoder_seq::get_rate() const {
        return m_pimpl->m_player->sampleRate();
    }

    bool decoder_seq::rewind() {
        m_pimpl->m_player->reset();
        return true;
    }

    std::chrono::microseconds decoder_seq::duration() const {
        return {};
    }

    bool decoder_seq::seek_to_time([[maybe_unused]] std::chrono::microseconds pos) {
        return false;
    }

    size_t decoder_seq::do_decode(float buf[], size_t len, [[maybe_unused]] bool& call_again) {
        if (m_pimpl->m_player->atEnd()) {
            return 0;
        }
        m_pimpl->m_player->generate(buf, len/2);
        return len;
    }
}
