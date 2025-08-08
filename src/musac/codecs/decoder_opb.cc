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
        return {};
    }

    bool decoder_opb::seek_to_time([[maybe_unused]] std::chrono::microseconds pos) {
        return false;
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
