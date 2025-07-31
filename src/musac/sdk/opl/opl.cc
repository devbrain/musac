//
// Created by igor on 3/19/25.
//

#include <limits>

#include <musac/sdk/opl/opl.hh>
#include "musac/sdk/opl/impl_opl.h"
namespace musac {
    opl::opl() {
        m_chip = opl_create();
    }

    opl::~opl() {
        opl_destroy(m_chip);
    }

    void opl::render(int16_t* buffer, size_t sample_pairs, float volume) const {
        if (sample_pairs == 0 || sample_pairs >= std::numeric_limits<int>::max()) {
            return;
        }
        opl_render(m_chip, buffer, static_cast<int>(sample_pairs), volume);
    }

    void opl::write(size_t count, uint16_t* regs, uint8_t* data) {
        if (count == 0 || count >= std::numeric_limits<int>::max()) {
            return;
        }
        opl_write(m_chip, static_cast<int>(count), regs, data);
    }

    void opl::write(uint16_t reg, uint8_t val) {
        opl_write(m_chip, 1, &reg, &val);
    }

    void opl::write(const opl_command& cmd) {
        opl_write(m_chip, 1, const_cast<uint16_t*>(&cmd.Addr), const_cast<uint8_t*>(&cmd.Data));
    }

    void opl::midi_notes_clear() {
        opl_clear(m_chip);
    }

    void opl::midi_note_on(int channel, int note, int velocity) {
        opl_midi_noteon(m_chip, channel, note, velocity);
    }

    void opl::midi_note_off(int channel, int note) {
        opl_midi_noteoff(m_chip, channel, note);
    }

    void opl::midi_pitchwheel(int channel, int wheelvalue) {
        opl_midi_pitchwheel(m_chip, channel, wheelvalue);
    }

    void opl::midi_controller(int channel, int id, int value) {
        opl_midi_controller(m_chip, channel, id, value);
    }

    void opl::midi_changeprog(int channel, int program) {
        opl_midi_changeprog(m_chip, channel, program);
    }
}
