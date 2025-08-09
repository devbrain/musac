
#include "sequence_hmp.h"
#include <cstring>

namespace musac {
#define READ_U32LE(data, pos) ((data[pos+3] << 24) | (data[pos+2] << 16) | (data[pos+1] << 8) | data[pos])

    class hmp_track : public mid_track {
    public:
        hmp_track(const uint8_t* data, size_t size, midi_sequence_hmp* sequence);

    protected:
        uint32_t read_delay() override;
    };

    // ----------------------------------------------------------------------------
    hmp_track::hmp_track(const uint8_t* data, size_t size, midi_sequence_hmp* sequence)
        : mid_track(data, size, sequence) {
        m_use_running_status = false;
    }

    // ----------------------------------------------------------------------------
    uint32_t hmp_track::read_delay() {
        uint32_t delay = 0;
        uint8_t data = 0;
        unsigned shift = 0;

        do {
            data = m_data[m_pos++];
            delay |= ((data & 0x7f) << shift);
            shift += 7;
        }
        while (!(data & 0x80) && (m_pos < m_size));

        return delay;
    }

    // ----------------------------------------------------------------------------
    midi_sequence_hmp::midi_sequence_hmp()
        : midi_sequence_mid() {
        m_type = 1;
        m_ticks_per_beat = 120;
        m_ticks_per_sec = 120;
    }

    // ----------------------------------------------------------------------------
    midi_sequence_hmp::~midi_sequence_hmp() = default;

    // ----------------------------------------------------------------------------
    void midi_sequence_hmp::read(const uint8_t* data, size_t size) {
        uint32_t num_tracks = READ_U32LE(data, 0x30);

        m_ticks_per_beat = READ_U32LE(data, 0x34);
        m_ticks_per_sec = READ_U32LE(data, 0x38);

        // longer signature = extended format
        uint32_t offset = (data[8] == 0) ? 0x308 : 0x388;

        for (int i = 0; i < num_tracks; i++) {
            if (offset + 12 >= size)
                break;

            uint32_t track_len = READ_U32LE(data, offset + 4);
            if (offset + track_len >= size)
                // try to handle a malformed/truncated chunk
                    track_len = (size - offset);

            if (track_len <= 12)
                break;

            m_tracks.push_back(new hmp_track(data + offset + 12, track_len - 12, this));

            offset += track_len;
        }
    }

    // ----------------------------------------------------------------------------
    bool midi_sequence_hmp::is_valid(const uint8_t* data, size_t size) {
        if (size < 0x40)
            return false;

        return !memcmp(data, "HMIMIDIP", 8);
    }

    // ----------------------------------------------------------------------------
    void midi_sequence_hmp::set_time_per_beat([[maybe_unused]] uint32_t usec) {
        // ?
    }

} // namespace musac