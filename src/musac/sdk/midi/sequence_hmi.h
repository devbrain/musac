#ifndef MUSAC_MIDI_SEQUENCE_HMI_H
#define MUSAC_MIDI_SEQUENCE_HMI_H

#include "sequence_mid.h"

namespace musac {
    class midi_sequence_hmi : public midi_sequence_mid
    {
    public:
        midi_sequence_hmi();
        ~midi_sequence_hmi() override;

        void set_time_per_beat(uint32_t usec) override;

        static bool is_valid(const uint8_t* data, size_t size);

    private:
        void read(const uint8_t* data, size_t size) override;
    };

} // namespace musac

#endif // MUSAC_MIDI_SEQUENCE_HMI_H
