#ifndef MUSAC_MIDI_SEQUENCE_MUS_H
#define MUSAC_MIDI_SEQUENCE_MUS_H

#include "sequence.h"

namespace musac {
    class midi_sequence_mus : public midi_sequence_impl
    {
    public:
        midi_sequence_mus();

        void reset() override;
        uint32_t update(opl_midi_synth& player) override;

        static bool is_valid(const uint8_t* data, size_t size);

    private:
        void read(const uint8_t* data, size_t size) override;
        void set_defaults();

        uint8_t m_data[1 << 16]{};
        uint16_t m_pos{};
        uint8_t m_last_vol[16]{};
    };

} // namespace musac

#endif // MUSAC_MIDI_SEQUENCE_MUS_H
