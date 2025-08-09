#include "sequence_mus.h"
#include <musac/sdk/midi/opl_midi_synth.hh>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace musac {
    // ----------------------------------------------------------------------------
    midi_sequence_mus::midi_sequence_mus()
        : midi_sequence_impl()
    {
        // cheap safety measure - fill the whole song buffer w/ "end of track" commands
        // (m_pos is 16 bits, so a malformed track will either hit this or just wrap around)
        std::fill(std::begin(m_data), std::end(m_data), 0x60);
        set_defaults();
    }

    // ----------------------------------------------------------------------------
    bool midi_sequence_mus::is_valid(const uint8_t* data, size_t size)
    {
        if (size < 8)
            return false;
        return !memcmp(data, "MUS\x1a", 4);
    }

    // ----------------------------------------------------------------------------
    void midi_sequence_mus::read(const uint8_t* data, size_t size)
    {
        if (size > 8)
        {
            uint16_t length = data[4] | (data[5] << 8);
            uint16_t pos    = data[6] | (data[7] << 8);

            if (pos < size)
            {
                if (pos + length > size)
                    length = size - pos;
                std::copy(data + pos, data + pos + length, m_data);
            }
        }
    }

    // ----------------------------------------------------------------------------
    void midi_sequence_mus::reset()
    {
        midi_sequence_impl::reset();
        set_defaults();
    }

    // ----------------------------------------------------------------------------
    void midi_sequence_mus::set_defaults()
    {
        m_pos = 0;
        std::fill(std::begin(m_last_vol), std::end(m_last_vol), 0x7f);
    }

    // ----------------------------------------------------------------------------
    uint32_t midi_sequence_mus::update(opl_midi_synth& player)
    {
        uint8_t event, channel, data, param;
        uint16_t last_pos;

        m_at_end = false;

        do
        {
            last_pos = m_pos;
            event = m_data[m_pos++];
            channel = event & 0xf;

            // map MUS channels to MIDI channels
            // (don't bother with the primary/secondary channel thing unless we need to)
            if (channel == 15) // percussion
                channel = 9;
            else if (channel >= 9)
                channel++;

            switch ((event >> 4) & 0x7)
            {
                case 0: // note off
                    player.midi_note_off(channel, m_data[m_pos++]);
                    break;

                case 1: // note on
                    data = m_data[m_pos++];
                    if (data & 0x80)
                        m_last_vol[channel] = m_data[m_pos++];
                    player.midi_note_on(channel, data, m_last_vol[channel]);
                    break;

                case 2: // pitch bend
                    player.midi_pitch_control(channel, (m_data[m_pos++] / 128.0) - 1.0);
                    break;

                case 3: // system event (channel mode messages)
                    data = m_data[m_pos++] & 0x7f;
                    switch (data)
                    {
                        case 10: player.midi_control_change(channel, 120, 0); break; // all sounds off
                        case 11: player.midi_control_change(channel, 123, 0); break; // all notes off
                        case 12: player.midi_control_change(channel, 126, 0); break; // mono on
                        case 13: player.midi_control_change(channel, 127, 0); break; // poly on
                        case 14: player.midi_control_change(channel, 121, 0); break; // reset all controllers
                        default: break;
                    }
                    break;

                case 4: // controller
                    data  = m_data[m_pos++] & 0x7f;
                    param = m_data[m_pos++];
                    // clamp CC param value - some tracks from tnt.wad have bad volume CCs
                    if (param > 0x7f)
                        param = 0x7f;
                    switch (data)
                    {
                        case 0: player.midi_program_change(channel, param); break;
                        case 1: player.midi_control_change(channel, 0,  param); break; // bank select
                        case 2: player.midi_control_change(channel, 1,  param); break; // mod wheel
                        case 3: player.midi_control_change(channel, 7,  param); break; // volume
                        case 4: player.midi_control_change(channel, 10, param); break; // pan
                        case 5: player.midi_control_change(channel, 11, param); break; // expression
                        case 6: player.midi_control_change(channel, 91, param); break; // reverb
                        case 7: player.midi_control_change(channel, 93, param); break; // chorus
                        case 8: player.midi_control_change(channel, 64, param); break; // sustain pedal
                        case 9: player.midi_control_change(channel, 67, param); break; // soft pedal
                        default: break;
                    }
                    break;

                case 5: // end of measure
                    break;

                case 6: // end of track
                    reset();
                    m_at_end = true;
                    return 0;

                case 7: // unused
                    m_pos++;
                    break;
            }
        } while (!(event & 0x80) && (m_pos > last_pos));

        // read delay in ticks, convert to # of samples
        uint32_t tick_delay = 0;
        do
        {
            event = m_data[m_pos++];
            tick_delay <<= 7;
            tick_delay |= (event & 0x7f);
        } while ((event & 0x80) && (m_pos > last_pos));

        if (m_pos < last_pos)
        {
            // premature end of track, 16 bit position overflowed
            reset();
            m_at_end = true;
            return 0;
        }

        double samples_per_tick = player.sample_rate() / 140.0;
        return round(tick_delay * samples_per_tick);
    }

} // namespace musac