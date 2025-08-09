
#include "sequence_mid.h"
#include <musac/sdk/midi/opl_midi_synth.hh>
#include <cmath>
#include <cstring>
#include <climits>

namespace musac {
#define READ_U16BE(data, pos) ((data[pos] << 8) | data[pos+1])
#define READ_U24BE(data, pos) ((data[pos] << 16) | (data[pos+1] << 8) | data[pos+2])
#define READ_U32BE(data, pos) ((data[pos] << 24) | (data[pos+1] << 16) | (data[pos+2] << 8) | data[pos+3])

    // ----------------------------------------------------------------------------
    mid_track::mid_track(const uint8_t* data, size_t size, midi_sequence_mid* sequence)
    {
        m_data = new uint8_t[size];
        m_size = size;
        memcpy(m_data, data, size);
        m_sequence = sequence;

        m_init_delay = true;
        m_use_running_status = true;
        m_use_note_duration = false;

        reset();
    }

    // ----------------------------------------------------------------------------
    mid_track::~mid_track()
    {
        delete[] m_data;
    }

    // ----------------------------------------------------------------------------
    void mid_track::reset()
    {
        m_pos = m_delay = 0;
        m_at_end = false;
        m_status = 0x00;
    }

    // ----------------------------------------------------------------------------
    void mid_track::advance(uint32_t time)
    {
        if (m_at_end)
            return;

        m_delay -= time;
        if (m_use_note_duration)
            for (auto& note : m_notes)
                note.delay -= time;
    }

    // ----------------------------------------------------------------------------
    uint32_t mid_track::read_vlq()
    {
        uint32_t vlq = 0;
        uint8_t data = 0;

        do
        {
            data = m_data[m_pos++];
            vlq <<= 7;
            vlq |= (data & 0x7f);
        } while ((data & 0x80) && (m_pos < m_size));

        return vlq;
    }

    // ----------------------------------------------------------------------------
    int32_t mid_track::min_delay()
    {
        int32_t delay = m_delay;
        if (m_use_note_duration)
            for (auto& note : m_notes)
                delay = std::min(delay, note.delay);
        return delay;
    }

    // ----------------------------------------------------------------------------
    uint32_t mid_track::update(opl_midi_synth& player)
    {
        if (m_init_delay && !m_pos)
        {
            m_delay = read_delay();
        }

        if (m_use_note_duration)
        {
            for (int i = 0; i < m_notes.size();)
            {
                if (m_notes[i].delay <= 0)
                {
                    player.midi_note_off(m_notes[i].channel, m_notes[i].note);
                    m_notes[i] = m_notes.back();
                    m_notes.pop_back();
                }
                else
                    i++;
            }
        }

        while (m_delay <= 0)
        {
            uint8_t data[2];
            mid_note note;

            // make sure we have enough data left for one full event
            if (m_size - m_pos < 3)
            {
                m_at_end = true;
                return UINT_MAX;
            }

            if (!m_use_running_status || (m_data[m_pos] & 0x80))
                m_status = m_data[m_pos++];

            switch (m_status >> 4)
            {
                case 9: // note on
                    data[0] = m_data[m_pos++];
                    data[1] = m_data[m_pos++];
                    player.midi_event(m_status, data[0], data[1]);

                    if (m_use_note_duration)
                    {
                        note.channel = m_status & 15;
                        note.note    = data[0];
                        note.delay   = read_vlq();
                        m_notes.push_back(note);
                    }
                    break;

                case 8:  // note off
                case 10: // polyphonic pressure
                case 11: // controller change
                case 14: // pitch bend
                    data[0] = m_data[m_pos++];
                    data[1] = m_data[m_pos++];
                    player.midi_event(m_status, data[0], data[1]);
                    break;

                case 12: // program change
                case 13: // channel pressure (ignored)
                    data[0] = m_data[m_pos++];
                    player.midi_event(m_status, data[0]);
                    break;

                case 15: // sysex / meta event
                    if (!meta_event(player))
                    {
                        m_at_end = true;
                        return UINT_MAX;
                    }
                    break;
            }

            m_delay += read_delay();
        }

        return min_delay();
    }

    // ----------------------------------------------------------------------------
    bool mid_track::meta_event(opl_midi_synth& player)
    {
        uint32_t len;

        if (m_status != 0xFF)
        {
            len = read_vlq();
            if (m_pos + len < m_size)
            {
                if (m_status == 0xf0)
                    player.midi_sysex(m_data + m_pos, len);
            }
            else
            {
                return false;
            }
        }
        else
        {
            uint8_t data = m_data[m_pos++];
            len = read_vlq();

            // end-of-track marker (or data just ran out)
            if (data == 0x2F || (m_pos + len >= m_size))
            {
                return false;
            }
            // tempo change
            if (data == 0x51)
            {
                m_sequence->set_time_per_beat(READ_U24BE(m_data, m_pos));
            }
        }

        m_pos += len;
        return true;
    }

    // ----------------------------------------------------------------------------
    midi_sequence_mid::midi_sequence_mid()
        : midi_sequence_impl()
    {
        m_type = 0;
        m_ticks_per_beat = 24;
        m_ticks_per_sec = 48;
    }

    // ----------------------------------------------------------------------------
    midi_sequence_mid::~midi_sequence_mid()
    {
        for (auto track : m_tracks)
            delete track;
    }

    // ----------------------------------------------------------------------------
    bool midi_sequence_mid::is_valid(const uint8_t* data, size_t size)
    {
        if (size < 12)
            return false;

        if (!memcmp(data, "MThd", 4))
        {
            uint32_t len = READ_U32BE(data, 4);
            if (len < 6) return false;

            uint16_t type = READ_U16BE(data, 8);
            if (type > 2) return false;

            return true;
        }
        else if (!memcmp(data, "RIFF", 4)
              && !memcmp(data + 8, "RMID", 4))
        {
            return true;
        }

        return false;
    }

    // ----------------------------------------------------------------------------
    void midi_sequence_mid::read(const uint8_t* data, size_t size)
    {
        // need at least the MIDI header + one track header
        if (size < 23)
            return;

        if (!memcmp(data, "RIFF", 4))
        {
            uint32_t offset = 12;
            while (offset + 8 < size)
            {
                const uint8_t* bytes = data + offset;
                uint32_t chunk_len = bytes[4] | (bytes[5] << 8) | (bytes[6] << 16) | (bytes[7] << 24);
                chunk_len = (chunk_len + 1) & ~1;

                // move to next subchunk
                offset += chunk_len + 8;
                if (offset > size)
                {
                    // try to handle a malformed/truncated chunk
                    chunk_len -= (offset - size);
                    offset = size;
                }

                if (!memcmp(bytes, "data", 4))
                {
                    if (is_valid(bytes + 8, chunk_len))
                        read(bytes + 8, chunk_len);
                    break;
                }
            }
        }
        else
        {
            uint32_t len = READ_U32BE(data, 4);

            m_type = READ_U16BE(data, 8);
            uint16_t num_tracks = READ_U16BE(data, 10);
            m_ticks_per_beat = READ_U16BE(data, 12);

            uint32_t offset = len + 8;
            for (unsigned i = 0; i < num_tracks; i++)
            {
                if (offset + 8 >= size)
                    break;

                const uint8_t* bytes = data + offset;
                if (memcmp(bytes, "MTrk", 4))
                    break;

                len = READ_U32BE(bytes, 4);
                offset += len + 8;
                if (offset > size)
                {
                    // try to handle a malformed/truncated chunk
                    len -= (offset - size);
                    offset = size;
                }

                m_tracks.push_back(new mid_track(bytes + 8, len, this));
            }
        }
    }

    // ----------------------------------------------------------------------------
    void midi_sequence_mid::reset()
    {
        midi_sequence_impl::reset();
        set_defaults();

        for (auto& track : m_tracks)
            track->reset();
    }

    // ----------------------------------------------------------------------------
    void midi_sequence_mid::set_defaults()
    {
        set_time_per_beat(500000);
    }

    // ----------------------------------------------------------------------------
    void midi_sequence_mid::set_time_per_beat(uint32_t usec)
    {
        double usec_per_tick = (double)usec / m_ticks_per_beat;
        m_ticks_per_sec = 1000000 / usec_per_tick;
    }

    // ----------------------------------------------------------------------------
    unsigned midi_sequence_mid::num_songs() const
    {
        if (m_type != 2)
            return 1;
        else
            return m_tracks.size();
    }

    // ----------------------------------------------------------------------------
    uint32_t midi_sequence_mid::update(opl_midi_synth& player)
    {
        uint32_t tick_delay = UINT_MAX;

        bool tracks_at_end = true;

        if (m_type != 2)
        {
            for (auto track : m_tracks)
            {
                if (!track->at_end())
                    tick_delay = std::min(tick_delay, track->update(player));
                tracks_at_end &= track->at_end();
            }
        }
        else if (m_song_num < m_tracks.size())
        {
            tick_delay   = m_tracks[m_song_num]->update(player);
            tracks_at_end = m_tracks[m_song_num]->at_end();
        }

        if (tracks_at_end)
        {
            reset();
            m_at_end = true;
            return 0;
        }

        m_at_end = false;

        for (auto track : m_tracks)
            track->advance(tick_delay);

        double samples_per_tick = player.sample_rate() / m_ticks_per_sec;
        return round(tick_delay * samples_per_tick);
    }

} // namespace musac