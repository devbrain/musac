#ifndef MUSAC_MIDI_SEQUENCE_MID_H
#define MUSAC_MIDI_SEQUENCE_MID_H

#include "sequence.h"
#include <vector>

namespace musac {
    class midi_sequence_mid;

    class mid_track
    {
    public:
        mid_track(const uint8_t* data, size_t size, midi_sequence_mid* sequence);
        virtual ~mid_track();

        void reset();
        void advance(uint32_t time);
        uint32_t update(opl_midi_synth& player);

        bool at_end() const { return m_at_end; }

    protected:
        uint32_t read_vlq();
        virtual uint32_t read_delay() { return read_vlq(); }
        int32_t min_delay();
        virtual bool meta_event(opl_midi_synth& player);

        midi_sequence_mid* m_sequence;
        uint8_t* m_data;
        uint32_t m_pos, m_size;
        int32_t m_delay;
        bool m_at_end;
        uint8_t m_status; // for MIDI running status

        // these are used for format-specific track data details
        bool m_init_delay; // true if there is an initial delay value at the start of the track
        bool m_use_running_status; // true if running status is supported by this format
        bool m_use_note_duration; // true if note on events are followed by a length

        struct mid_note
        {
            uint8_t channel, note;
            int32_t delay;
        };
        std::vector<mid_note> m_notes;
    };

    class midi_sequence_mid : public midi_sequence_impl
    {
    public:
        midi_sequence_mid();
        ~midi_sequence_mid() override;

        void reset() override;
        uint32_t update(opl_midi_synth& player) override;

        virtual void set_time_per_beat(uint32_t usec);

        unsigned num_songs() const override;

        static bool is_valid(const uint8_t* data, size_t size);

    protected:
        std::vector<mid_track*> m_tracks;

        uint16_t m_type;
        uint16_t m_ticks_per_beat;
        double m_ticks_per_sec;

    private:
        void read(const uint8_t* data, size_t size) override;
        virtual void set_defaults();
    };

} // namespace musac

#endif // MUSAC_MIDI_SEQUENCE_MID_H
