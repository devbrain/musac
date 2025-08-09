#ifndef MUSAC_MIDI_SEQUENCE_XMI_H
#define MUSAC_MIDI_SEQUENCE_XMI_H

#include "sequence_mid.h"

namespace musac {
    class midi_sequence_xmi : public midi_sequence_mid
    {
    public:
        midi_sequence_xmi();
        ~midi_sequence_xmi() override;

        void set_time_per_beat(uint32_t usec) override;

        static bool is_valid(const uint8_t* data, size_t size);

    private:
        void read(const uint8_t* data, size_t size) override;
        uint32_t read_root_chunk(const uint8_t* data, size_t size);
    };

} // namespace musac

#endif // MUSAC_MIDI_SEQUENCE_XMI_H
