//
// Created by igor on 3/20/25.
//

#include <musac/codecs/decoder_cmf.hh>
#include <musac/sdk/samples_converter.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include "musac/codecs/cmf/fmdrv.h"
#include <vector>

namespace musac {
    struct decoder_cmf::impl {
        impl ()
            : m_status(0),
              m_total_samples(0),
              m_current_sample(0) {
            m_decoder = sbfm_init(44100);
            sbfm_reset(m_decoder);
            sbfm_status_addx(m_decoder, &m_status);
        }

        ~impl () {
            sbfm_destroy(m_decoder);
        }
        fmdrv_s* m_decoder;
        std::vector<uint8_t> m_song;
        uint8_t m_status;
        uint32_t m_total_samples;
        uint32_t m_current_sample;

    };

    decoder_cmf::decoder_cmf()
        : m_pimpl(std::make_unique<impl>()){
    }

    decoder_cmf::~decoder_cmf() = default;

    bool decoder_cmf::do_accept(io_stream* rwops) {
        // Check for "CTMF" magic bytes at the start of the file
        uint8_t magic[4];
        
        if (rwops->read(magic, 4) != 4) {
            return false;
        }
        
        return (magic[0] == 'C' && magic[1] == 'T' && magic[2] == 'M' && magic[3] == 'F');
    }
    
    const char* decoder_cmf::get_name() const {
        return "CMF (Creative Music File)";
    }

    void decoder_cmf::open(io_stream* rwops) {
        auto sz = rwops->get_size();
        if (sz <= 0) {
            THROW_RUNTIME("Invalid CMF file size");
        }
        m_pimpl->m_song.resize((size_t)sz);
        rwops->read( m_pimpl->m_song.data(), (size_t)sz);
        auto* data = m_pimpl->m_song.data();
        
        // Validate minimum size for CMF header
        if (sz < 0x26) {
            THROW_RUNTIME("CMF file too small for header");
        }
        
        // Check for "CTMF" header
        if (data[0] != 'C' || data[1] != 'T' || data[2] != 'M' || data[3] != 'F') {
            THROW_RUNTIME("Invalid CMF file header");
        }
        
        // Check that speed value is not zero to avoid division by zero
        uint16_t speed_value = READ_16LE(&data[0x0c]);
        if (speed_value == 0) {
            THROW_RUNTIME("CMF speed value is zero");
        }
        
        // Validate offsets before using them
        uint16_t instr_offset = READ_16LE(&data[0x06]);
        uint16_t music_offset = READ_16LE(&data[0x08]);
        uint16_t instr_count = READ_16LE(&data[0x24]);
        
        if (instr_offset >= sz || music_offset >= sz) {
            THROW_RUNTIME("CMF file has invalid offsets");
        }
        
        // Create a temporary decoder just for duration calculation
        fmdrv_s* temp_decoder = sbfm_init(44100);
        sbfm_reset(temp_decoder);
        sbfm_instrument(temp_decoder, &data[instr_offset], instr_count);
        sbfm_song_speed(temp_decoder, (uint16_t)(0x1234dc / speed_value));
        sbfm_play_music(temp_decoder, &data[music_offset]);
        
        // Calculate total duration using the temporary decoder
        m_pimpl->m_total_samples = sbfm_calculate_duration_samples(temp_decoder);
        m_pimpl->m_current_sample = 0;
        
        // Destroy the temporary decoder
        sbfm_destroy(temp_decoder);
        
        // Now set up the real decoder for playback
        sbfm_instrument(m_pimpl->m_decoder, &data[instr_offset], instr_count);
        sbfm_song_speed(m_pimpl->m_decoder, (uint16_t)(0x1234dc / speed_value));
        sbfm_play_music(m_pimpl->m_decoder, &data[music_offset]);
        
        set_is_open(true);
    }

    channels_t decoder_cmf::get_channels() const {
        return 2;
    }

    sample_rate_t decoder_cmf::get_rate() const {
        //int r = 44100;
        return 44100;
    }

    bool decoder_cmf::rewind() {
        if (!is_open()) {
            return false;
        }
        
        m_pimpl->m_current_sample = 0;
        auto* data = m_pimpl->m_song.data();
        
        // Extract required values from the CMF header
        uint16_t instr_offset = READ_16LE(&data[0x06]);
        uint16_t music_offset = READ_16LE(&data[0x08]);
        uint16_t instr_count = READ_16LE(&data[0x24]);
        uint16_t speed_value = READ_16LE(&data[0x0c]);
        
        // Reset and reinitialize the decoder
        sbfm_reset(m_pimpl->m_decoder);
        sbfm_instrument(m_pimpl->m_decoder, &data[instr_offset], instr_count);
        sbfm_song_speed(m_pimpl->m_decoder, (uint16_t)(0x1234dc / speed_value));
        sbfm_play_music(m_pimpl->m_decoder, &data[music_offset]);
        
        return true;
    }

    std::chrono::microseconds decoder_cmf::duration() const {
        if (!is_open() || m_pimpl->m_total_samples == 0) {
            return std::chrono::microseconds(0);
        }
        
        // Calculate duration based on 44100 Hz sample rate
        double duration_seconds = static_cast<double>(m_pimpl->m_total_samples) / 44100.0;
        return std::chrono::microseconds(
            static_cast<int64_t>(duration_seconds * 1'000'000)
        );
    }

    bool decoder_cmf::seek_to_time([[maybe_unused]] std::chrono::microseconds pos) {
        if (!is_open()) {
            return false;
        }
        
        // Convert time to sample position
        double seconds = static_cast<double>(pos.count()) / 1'000'000.0;
        uint32_t target_sample = static_cast<uint32_t>(seconds * 44100);
        
        // Clamp to valid range
        if (target_sample > m_pimpl->m_total_samples) {
            target_sample = m_pimpl->m_total_samples;
        }
        
        auto* data = m_pimpl->m_song.data();
        
        // Extract required values from the CMF header
        uint16_t instr_offset = READ_16LE(&data[0x06]);
        uint16_t music_offset = READ_16LE(&data[0x08]);
        uint16_t instr_count = READ_16LE(&data[0x24]);
        uint16_t speed_value = READ_16LE(&data[0x0c]);
        
        // Use the seek function to jump to the target position
        if (sbfm_seek_to_sample(m_pimpl->m_decoder, target_sample) != 0) {
            // If seek fails, reset and reinitialize
            sbfm_reset(m_pimpl->m_decoder);
            sbfm_instrument(m_pimpl->m_decoder, &data[instr_offset], instr_count);
            sbfm_song_speed(m_pimpl->m_decoder, (uint16_t)(0x1234dc / speed_value));
            sbfm_play_music(m_pimpl->m_decoder, &data[music_offset]);
            
            // Fast-forward to the target position
            sbfm_seek_to_sample(m_pimpl->m_decoder, target_sample);
        }
        
        m_pimpl->m_current_sample = target_sample;
        return true;
    }

    size_t decoder_cmf::do_decode(float buf[], size_t len, [[maybe_unused]] bool& call_again) {
        size_t total = 0;

        float* stream = buf;
        for (size_t i = 0; i < len/2; i++) {
            sbfm_render_stereo(m_pimpl->m_decoder, stream, stream + 1);

            stream += 2;
            total++;
            m_pimpl->m_current_sample++;
            
            if (!m_pimpl->m_status) {
                break;
            }
        }
        return 2*total;
    }
}
