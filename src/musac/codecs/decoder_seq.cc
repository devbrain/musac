//
// Created by igor on 3/23/25.
//

#include <musac/codecs/decoder_seq.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include <musac/sdk/midi/opl_midi_synth.hh>
#include <musac/sdk/midi/opl_patches.hh>
#include <musac/sdk/midi/midi_opl_data.h>
#include <memory>

namespace musac {
    struct decoder_seq::impl {
        impl() : m_player(std::make_unique<opl_midi_synth>()) {
            m_player->load_patches(GENMIDI_wopl, GENMIDI_wopl_size);
            m_player->set_loop(false);
            unsigned int sampleRate = 44100;
            double gain = 1.0;
            double filter = 5.0;
	        m_player->set_sample_rate(sampleRate);
	        m_player->set_gain(gain);
	        m_player->set_filter(filter);
	        m_player->set_stereo(true);
        }
        ~impl() = default;
        std::unique_ptr<opl_midi_synth> m_player;
    };

    decoder_seq::decoder_seq()
        : m_pimpl(std::make_unique<impl>()){
    }

    decoder_seq::~decoder_seq() = default;

    void decoder_seq::open(io_stream* rwops) {
        bool result = m_pimpl->m_player->load_sequence(rwops);
        if (!result) {
            THROW_RUNTIME("Failed to load SEQ file");
        }
        set_is_open(true);
    }

    channels_t decoder_seq::get_channels() const {
        return m_pimpl->m_player->stereo() ? 2 : 1;
    }

    sample_rate_t decoder_seq::get_rate() const {
        return m_pimpl->m_player->sample_rate();
    }

    bool decoder_seq::rewind() {
        m_pimpl->m_player->reset();
        return true;
    }

    std::chrono::microseconds decoder_seq::duration() const {
        if (!is_open()) {
            return std::chrono::microseconds(0);
        }
        
        // Calculate duration using the MIDI synth's fast-forward capability
        uint64_t total_samples = m_pimpl->m_player->calculate_duration_samples();
        if (total_samples == 0) {
            return std::chrono::microseconds(0);
        }
        
        // Convert samples to microseconds
        double sample_rate = m_pimpl->m_player->sample_rate();
        double duration_seconds = static_cast<double>(total_samples) / sample_rate;
        
        return std::chrono::microseconds(
            static_cast<int64_t>(duration_seconds * 1'000'000)
        );
    }

    bool decoder_seq::seek_to_time([[maybe_unused]] std::chrono::microseconds pos) {
        if (!is_open()) {
            return false;
        }
        
        // Convert time to sample position
        double sample_rate = m_pimpl->m_player->sample_rate();
        double seconds = pos.count() / 1'000'000.0;
        uint64_t target_sample = static_cast<uint64_t>(seconds * sample_rate);
        
        // Use the MIDI synth's seeking capability
        return m_pimpl->m_player->seek_to_sample(target_sample);
    }

    size_t decoder_seq::do_decode(float buf[], size_t len, [[maybe_unused]] bool& call_again) {
        if (m_pimpl->m_player->at_end()) {
            return 0;
        }
        m_pimpl->m_player->generate(buf, len/2);
        return len;
    }
}
