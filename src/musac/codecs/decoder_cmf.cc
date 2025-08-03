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
            : m_status(0){
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

    };

    decoder_cmf::decoder_cmf()
        : m_pimpl(std::make_unique<impl>()){
    }

    decoder_cmf::~decoder_cmf() = default;

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
        
        sbfm_instrument(m_pimpl->m_decoder, &data[instr_offset], instr_count);
        sbfm_song_speed(m_pimpl->m_decoder, (uint16_t)(0x1234dc / speed_value));
        sbfm_play_music(m_pimpl->m_decoder, &data[music_offset]);
        set_is_open(true);
    }

    unsigned int decoder_cmf::get_channels() const {
        return 2;
    }

    unsigned int decoder_cmf::get_rate() const {
        //int r = 44100;
        return 44100;
    }

    bool decoder_cmf::rewind() {
        return false;
    }

    std::chrono::microseconds decoder_cmf::duration() const {
        return {};
    }

    bool decoder_cmf::seek_to_time([[maybe_unused]] std::chrono::microseconds pos) {
        return false;
    }

    unsigned int decoder_cmf::do_decode(float buf[], unsigned int len, [[maybe_unused]] bool& call_again) {
        unsigned int total = 0;

        float* stream = buf;
        for (unsigned int i=0; i<len/2; i++) {
            sbfm_render_stereo(m_pimpl->m_decoder, stream, stream + 1);

            stream += 2;
            total++;
            if (!m_pimpl->m_status) {
                break;
            }
        }
        return 2*total;
    }
}
