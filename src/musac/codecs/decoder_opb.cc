//
// Created by igor on 3/19/25.
//

#include <musac/codecs/decoder_opb.hh>
#include <musac/sdk/opl/opl_player.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include "musac/codecs/opb_lib/opblib.h"



constexpr unsigned int SAMPLE_RATE = 44100;

namespace musac {
    struct decoder_opb::impl {
        impl()
            : m_player(SAMPLE_RATE) {
        }

        static int ReceiveOpbBuffer(OPB_Command* commandStream, size_t commandCount, void* context) {
            auto* self = static_cast<impl*>(context);
            self->m_player.copy(commandStream, commandCount);
            return 0;
        }

        opl_player m_player;
    };

    static size_t StreamReader(void* buffer, size_t elementSize, size_t elementCount, void* context) {
        auto rwops = static_cast<io_stream*>(context);
        rwops->read( buffer, elementSize * elementCount);
        return elementCount;
    }

    decoder_opb::decoder_opb()
        : m_pimpl(std::make_unique <impl>()) {
    }

    decoder_opb::~decoder_opb() = default;

    bool decoder_opb::accept(io_stream* rwops) {
        if (!rwops) {
            return false;
        }
        
        // Save current stream position
        auto original_pos = rwops->tell();
        if (original_pos < 0) {
            return false;
        }
        
        // Check for "OPBin1" magic bytes followed by null terminator
        uint8_t magic[7];
        bool result = false;
        
        if (rwops->read(magic, 7) == 7) {
            result = (magic[0] == 'O' && magic[1] == 'P' && magic[2] == 'B' &&
                     magic[3] == 'i' && magic[4] == 'n' && magic[5] == '1' && magic[6] == '\0');
        }
        
        // Restore original position
        rwops->seek(original_pos, seek_origin::set);
        return result;
    }
    
    const char* decoder_opb::get_name() const {
        return "OPB (OPL Binary)";
    }

    void decoder_opb::open(io_stream* rwops) {
        auto rc = OPB_BinaryToOpl(StreamReader, rwops, impl::ReceiveOpbBuffer, m_pimpl.get());
        if (rc != 0) {
            THROW_RUNTIME("Failed to load OPB file");
        }
        set_is_open(true);
    }

    channels_t decoder_opb::get_channels() const {
        return 2;
    }

    sample_rate_t decoder_opb::get_rate() const {
        return SAMPLE_RATE;
    }

    bool decoder_opb::rewind() {
        m_pimpl->m_player.rewind();
        return true;
    }

    std::chrono::microseconds decoder_opb::duration() const {
        if (!is_open()) {
            return std::chrono::microseconds(0);
        }
        
        double duration_seconds = m_pimpl->m_player.get_duration();
        return std::chrono::microseconds(
            static_cast<int64_t>(duration_seconds * 1'000'000)
        );
    }

    bool decoder_opb::seek_to_time([[maybe_unused]] std::chrono::microseconds pos) {
        if (!is_open()) {
            return false;
        }
        
        double seconds = static_cast<double>(pos.count()) / 1'000'000.0;
        return m_pimpl->m_player.seek(seconds);
    }

    size_t decoder_opb::do_decode(float* const buf, size_t len, bool& callAgain) {
        auto rc = m_pimpl->m_player.render(buf, len);
        if (rc == 0) {
            return rc;
        }
        if (rc < len) {
            callAgain = true;
        }
        return rc;
    }
}
