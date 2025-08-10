//
// Created by igor on 3/20/25.
//

#include <musac/codecs/decoder_vgm.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include <chrono>
#include "musac/codecs/vgm/vgm_player.hh"

constexpr unsigned int SAMPLE_RATE = 44100;

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

    bool decoder_vgm::accept(io_stream* rwops) {
        if (!rwops) {
            return false;
        }
        
        // Save current stream position
        auto original_pos = rwops->tell();
        if (original_pos < 0) {
            return false;
        }
        
        // Check for "Vgm " magic bytes at the start of the file
        // VGM files can be gzip compressed, so we need to check for both cases
        uint8_t magic[4];
        bool result = false;
        
        if (rwops->read(magic, 4) == 4) {
            // Check for uncompressed VGM signature "Vgm "
            if (magic[0] == 'V' && magic[1] == 'g' && magic[2] == 'm' && magic[3] == ' ') {
                result = true;
            }
            // Check for gzip compressed VGM (0x1F 0x8B 0x08)
            else if (magic[0] == 0x1f && magic[1] == 0x8b && magic[2] == 0x08) {
                // It's a gzip file, but we'd need to decompress to check if it's VGM
                // For now, we'll assume it could be VGM and let the full load process validate
                result = true;
            }
        }
        
        // Restore original position
        rwops->seek(original_pos, seek_origin::set);
        return result;
    }
    
    const char* decoder_vgm::get_name() const {
        return "VGM (Video Game Music)";
    }

    void decoder_vgm::open(io_stream* rwops) {
        bool result = m_pimpl->m_player.load(rwops);
        if (!result) {
            THROW_RUNTIME("Failed to load VGM file");
        }
        
        // Calculate actual duration using silent mode if header value is not reliable
        uint32_t header_samples = m_pimpl->m_player.get_total_samples();
        if (header_samples == 0) {
            // No duration in header, calculate it
            m_pimpl->m_player.calculate_duration_samples();
        }
        
        set_is_open(true);
    }

    channels_t decoder_vgm::get_channels() const {
        return 2;
    }

    sample_rate_t decoder_vgm::get_rate() const {
        return SAMPLE_RATE;
    }

    bool decoder_vgm::rewind() {
        if (!is_open()) {
            return false;
        }
        return m_pimpl->m_player.rewind();
    }

    std::chrono::microseconds decoder_vgm::duration() const {
        if (!is_open()) {
            return std::chrono::microseconds(0);
        }
        
        // Get total samples - either from header or calculated
        uint32_t total_samples = m_pimpl->m_player.get_total_samples();
        if (total_samples == 0) {
            // If header didn't have duration, calculate it dynamically
            // Note: This is const method so we can't cache the result
            const_cast<vgm_player&>(m_pimpl->m_player).calculate_duration_samples();
            total_samples = m_pimpl->m_player.get_total_samples();
        }
        
        if (total_samples == 0) {
            return std::chrono::microseconds(0);
        }
        
        // VGM files are always 44100 Hz
        double duration_seconds = static_cast<double>(total_samples) / 44100.0;
        
        return std::chrono::microseconds(
            static_cast<int64_t>(duration_seconds * 1'000'000)
        );
    }

    bool decoder_vgm::seek_to_time([[maybe_unused]] std::chrono::microseconds pos) {
        if (!is_open()) {
            return false;
        }
        
        // Convert time to sample position
        double seconds = static_cast<double>(pos.count()) / 1'000'000.0;
        uint32_t target_sample = static_cast<uint32_t>(seconds * 44100);
        
        return m_pimpl->m_player.seek_to_sample(target_sample);
    }

    size_t decoder_vgm::do_decode(float buf[], size_t len, bool& callAgain) {
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
