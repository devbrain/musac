

#include "sequence_hmi.h"
#include <cstring>

namespace musac {
#define READ_U16LE(data, pos) ((data[pos+1] << 8) | data[pos])
#define READ_U32LE(data, pos) ((data[pos+3] << 24) | (data[pos+2] << 16) | (data[pos+1] << 8) | data[pos])

    class hmi_track : public mid_track
    {
    public:
        hmi_track(const uint8_t* data, size_t size, midi_sequence_hmi* sequence);

    protected:
        bool meta_event(opl_midi_synth& player) override;
    };

    // ----------------------------------------------------------------------------
    hmi_track::hmi_track(const uint8_t* data, size_t size, midi_sequence_hmi* sequence)
        : mid_track(data, size, sequence)
    {
        m_use_note_duration = true;
    }

    // ----------------------------------------------------------------------------
    bool hmi_track::meta_event(opl_midi_synth& player)
    {
        if (m_status == 0xFE)
        {
            uint8_t data = m_data[m_pos++];

            if (data == 0x10)
            {
                if (m_pos + 7 >= m_size)
                    return false;

                m_pos += m_data[m_pos + 2] + 7;
            }
            else if (data == 0x12)
                m_pos += 2;
            else if (data == 0x13)
                m_pos += 10;
            else if (data == 0x14) // loop start
                m_pos += 2;
            else if (m_pos == 0x15) // loop end
                m_pos += 6;
            else
                return false;

            return m_pos < m_size;
        }
        else
        {
            return mid_track::meta_event(player);
        }
    }

    // ----------------------------------------------------------------------------
    midi_sequence_hmi::midi_sequence_hmi()
        : midi_sequence_mid()
    {
        m_type = 1;
    }

    // ----------------------------------------------------------------------------
    midi_sequence_hmi::~midi_sequence_hmi() = default;

    // ----------------------------------------------------------------------------
    void midi_sequence_hmi::read(const uint8_t* data, size_t size)
    {
        uint32_t num_tracks  = READ_U32LE(data, 0xE4);
        uint32_t track_table = READ_U32LE(data, 0xE8);

        m_ticks_per_beat = READ_U16LE(data, 0xD2);
        m_ticks_per_sec  = READ_U16LE(data, 0xD4);

        for (int i = 0; i < num_tracks; i++)
        {
            uint32_t track_ptr = track_table + 4*i;

            if (track_ptr + 8 >= size)
                break;

            uint32_t offset = READ_U32LE(data, track_ptr);
            if (offset >= size)
                continue;

            uint32_t track_len = READ_U32LE(data, track_ptr + 4) - offset;

            if (((offset + track_len) > size) || (i == num_tracks - 1))
                // stop at the end of the file if this is the last track
                    // (or if the track is just malformed/truncated)
                        track_len = size - offset;
            if ((track_len <= 0x5b) || memcmp(data + offset, "HMI-MIDITRACK", 13))
                continue;

            uint32_t track_start = READ_U32LE(data, offset + 0x57);
            if (track_start < 0x5b)
                continue;

            m_tracks.push_back(new hmi_track(data + offset + track_start, track_len - track_start, this));
        }
    }

    // ----------------------------------------------------------------------------
    bool midi_sequence_hmi::is_valid(const uint8_t* data, size_t size)
    {
        if (size < 0xEC)
            return false;

        return !memcmp(data, "HMI-MIDISONG061595", 18);
    }

    // ----------------------------------------------------------------------------
    void midi_sequence_hmi::set_time_per_beat([[maybe_unused]] uint32_t usec)
    {
        // ?
    }

} // namespace musac