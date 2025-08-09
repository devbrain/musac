#ifndef MUSAC_MIDI_SEQUENCE_HMP_H
#define MUSAC_MIDI_SEQUENCE_HMP_H

#include "sequence_mid.h"

namespace musac {
    class midi_sequence_hmp : public midi_sequence_mid
    {
    public:
        midi_sequence_hmp();
        ~midi_sequence_hmp() override;

        void set_time_per_beat(uint32_t usec) override;

        static bool is_valid(const uint8_t* data, size_t size);

    private:
        void read(const uint8_t* data, size_t size) override;
    };

} // namespace musac

#endif // MUSAC_MIDI_SEQUENCE_HMP_H
