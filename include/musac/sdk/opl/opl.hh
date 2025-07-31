//
// Created by igor on 3/19/25.
//

#ifndef  OPL_HH
#define  OPL_HH

#include <cstdint>
#include <cstddef>
#include <musac/sdk/opl/opl_command.h>
struct opl_t;

namespace musac {
    class opl {
        public:
            opl();
            ~opl();

            opl(const opl&) = delete;
            opl& operator=(const opl&) = delete;

            void render(int16_t* buffer, size_t sample_pairs, float volume) const;

            void write(size_t count, uint16_t* regs, uint8_t* data);
            void write(uint16_t reg, uint8_t val);
            void write(const opl_command& cmd);

            // Notes API
            /* turns off all notes */
            void midi_notes_clear();

            /* turn note 'on', on emulated MIDI channel */
            void midi_note_on(int channel, int note, int velocity );

            /* turn note 'off', on emulated MIDI channel */
            void midi_note_off(int channel, int note );

            /* adjust the pitch wheel on emulated MIDI channel */
            void midi_pitchwheel(int channel, int wheelvalue );

            /* emulate MIDI 'controller' messages on the OPL */
            void midi_controller(int channel, int id, int value );

            /* assign a new instrument to emulated MIDI channel */
            void midi_changeprog(int channel, int program );

        private:
            opl_t* m_chip;
    };
}

#endif
