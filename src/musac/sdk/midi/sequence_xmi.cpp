#include "sequence_xmi.h"
#include <cstring>

namespace musac {
#define READ_U16BE(data, pos) ((data[pos] << 8) | data[pos+1])
#define READ_U24BE(data, pos) ((data[pos] << 16) | (data[pos+1] << 8) | data[pos+2])
#define READ_U32BE(data, pos) ((data[pos] << 24) | (data[pos+1] << 16) | (data[pos+2] << 8) | data[pos+3])

    class xmi_track : public mid_track
    {
    public:
        xmi_track(const uint8_t* data, size_t size, midi_sequence_xmi* sequence);

    protected:
        uint32_t read_delay() override;
    };

    // ----------------------------------------------------------------------------
    xmi_track::xmi_track(const uint8_t* data, size_t size, midi_sequence_xmi* sequence)
        : mid_track(data, size, sequence)
    {
        m_init_delay = false;
        m_use_running_status = false;
        m_use_note_duration = true;
    }

    // ----------------------------------------------------------------------------
    uint32_t xmi_track::read_delay()
    {
        uint32_t delay = 0;
        uint8_t data = 0;

        if (m_pos >= m_size || (m_data[m_pos] & 0x80))
            return 0;

        do
        {
            data = m_data[m_pos];
            if (!(data & 0x80))
            {
                delay += data;
                m_pos++;
            }
        } while ((data == 0x7f) && (m_pos < m_size));

        return delay;
    }

    // ----------------------------------------------------------------------------
    midi_sequence_xmi::midi_sequence_xmi()
        : midi_sequence_mid()
    {
        m_type = 2;
        m_ticks_per_beat = 0; // unused
        m_ticks_per_sec = 120;
    }

    // ----------------------------------------------------------------------------
    midi_sequence_xmi::~midi_sequence_xmi() = default;

    // ----------------------------------------------------------------------------
    void midi_sequence_xmi::read(const uint8_t* data, size_t size)
    {
        uint32_t chunk_size;
        while ((chunk_size = read_root_chunk(data, size)) != 0)
        {
            data += chunk_size;
            size -= chunk_size;
        }
    }

    // ----------------------------------------------------------------------------
    uint32_t midi_sequence_xmi::read_root_chunk(const uint8_t* data, size_t size)
    {
        // need at least a root chunk and one subchunk (and its contents)
        if (size > 12 + 8)
        {
            // length of the root chunk
            uint32_t root_len = READ_U32BE(data, 4);
            root_len = (root_len + 1) & ~1;
            // offset to the current sub-chunk
            uint32_t offset = 12;
            // offset to the data after the root chunk
            uint32_t root_end = std::min(root_len + 8, (uint32_t)size);

            if (!memcmp(data, "FORM", 4))
            {
                uint32_t chunk_len;

                while (offset < root_end)
                {
                    const uint8_t* bytes = data + offset;

                    chunk_len = READ_U32BE(bytes, 4);
                    chunk_len = (chunk_len + 1) & ~1;

                    // move to next subchunk
                    offset += chunk_len + 8;
                    if (offset > root_end)
                    {
                        // try to handle a malformed/truncated chunk
                        chunk_len -= (offset - root_end);
                        offset = root_end;
                    }

                    if (!memcmp(bytes, "EVNT", 4))
                        m_tracks.push_back(new xmi_track(bytes + 8, chunk_len, this));
                }
            }
            else if (!memcmp(data, "CAT ", 4))
            {
                while (offset < root_end)
                {
                    offset += read_root_chunk(data + offset, size - offset);
                }
            }

            return root_end;
        }

        return 0;
    }

    // ----------------------------------------------------------------------------
    bool midi_sequence_xmi::is_valid(const uint8_t* data, size_t size)
    {
        // need at least 2 root chunks and one EVNT chunk header
        if (size < 12)
            return false;

        if (memcmp(data,     "FORM", 4) != 0)
            return false;
        if (memcmp(data + 8, "XDIR", 4) != 0)
            return false;

        return true;
    }

    // ----------------------------------------------------------------------------
    void midi_sequence_xmi::set_time_per_beat(uint32_t usec)
    {
        double usec_per_tick = (double)usec / ((usec * 3) / 25000);
        m_ticks_per_sec = 1000000 / usec_per_tick;
    }

} // namespace musac