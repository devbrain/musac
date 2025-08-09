#ifndef MUSAC_OPL_MIDI_SYNTH_IMPL_HH
#define MUSAC_OPL_MIDI_SYNTH_IMPL_HH

#include <musac/sdk/midi/opl_midi_synth.hh>
#include <musac/sdk/midi/opl_patches.hh>
#include "internal/ymfm/ymfm_opl.h"
#include <climits>
#include <queue>
#include <vector>

namespace musac {

class midi_sequence_impl;

struct midi_channel {
    uint8_t num = 0;
    bool percussion = false;
    uint8_t bank = 0;
    uint8_t patch_num = 0;
    uint8_t volume = 127;
    uint8_t pan = 64;
    double base_pitch = 0.0; // pitch wheel position
    double pitch = 1.0; // frequency multiplier
    uint16_t rpn = 0x3fff;
    uint8_t bend_range = 2;
};

struct opl_voice {
    int chip = 0;
    const midi_channel* channel = nullptr;
    const opl_patch* patch = nullptr;
    const patch_voice* voice_patch = nullptr;

    uint16_t num = 0;
    uint16_t op = 0; // base operator number, set based on voice num.
    bool four_op_primary = false;
    opl_voice* four_op_other = nullptr;

    bool on = false;
    bool just_changed = false; // true after note on/off, false after generating at least 1 sample
    uint8_t note = 0;
    uint8_t velocity = 0;

    // block and F number, calculated from note and channel pitch
    uint16_t freq = 0;

    // how long has this note been playing (incremented each midi update)
    uint32_t duration = UINT_MAX;
};

class opl_midi_synth::impl : public ymfm::ymfm_interface {
public:
    impl(int num_chips, opl_midi_synth::chip_type type, opl_midi_synth* parent);
    virtual ~impl();

    void set_loop(bool loop) { m_looping = loop; }
    void set_sample_rate(sample_rate_t rate);
    void set_gain(double gain);
    void set_filter(double cutoff);
    void set_stereo(bool on = true);

    bool load_sequence(const char* path);
    bool load_sequence(FILE* file, int offset = 0, size_t size = 0);
    bool load_sequence(io_stream* file, int offset = 0, size_t size = 0);
    bool load_sequence(const uint8_t* data, size_t size);

    bool load_patches(const char* path);
    bool load_patches(FILE* file, int offset = 0, size_t size = 0);
    bool load_patches(const uint8_t* data, size_t size);

    void generate(float* data, unsigned num_samples);
    void generate(int16_t* data, unsigned num_samples);

    void reset();
    bool at_end() const;
    
    void set_song_num(unsigned num);
    unsigned num_songs() const;
    unsigned song_num() const;

    void midi_event(uint8_t status, uint8_t data0, uint8_t data1 = 0);
    void midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void midi_note_off(uint8_t channel, uint8_t note);
    void midi_pitch_control(uint8_t channel, double pitch);
    void midi_program_change(uint8_t channel, uint8_t patch_num);
    void midi_control_change(uint8_t channel, uint8_t control, uint8_t value);
    void midi_sysex(const uint8_t* data, uint32_t length);

    static double midi_calc_bend(double semitones);

    sample_rate_t sample_rate() const { return m_sample_rate; }
    opl_midi_synth::chip_type chip_type() const { return m_chip_type; }
    bool stereo() const { return m_stereo; }
    const std::string& patch_name(uint8_t num) { return m_patches[num].name; }

private:
    static const unsigned master_clock = 14318181;

    enum {
        REG_TEST        = 0x01,
        REG_OP_MODE     = 0x20,
        REG_OP_LEVEL    = 0x40,
        REG_OP_AD       = 0x60,
        REG_OP_SR       = 0x80,
        REG_VOICE_FREQL = 0xA0,
        REG_VOICE_FREQH = 0xB0,
        REG_VOICE_CNT   = 0xC0,
        REG_OP_WAVEFORM = 0xE0,
        REG_4OP         = 0x104,
        REG_NEW         = 0x105,
    };

    void update_midi();
    void run_samples(int chip, unsigned count);
    void write(int chip, uint16_t addr, uint8_t data);

    opl_voice* find_voice(uint8_t channel, const opl_patch* patch, uint8_t note);
    opl_voice* find_voice(uint8_t channel, uint8_t note, bool just_changed = false);
    const opl_patch* find_patch(uint8_t channel, uint8_t note) const;
    bool use_four_op(const opl_patch* patch) const;
    std::pair<bool, bool> active_carriers(const opl_voice& voice) const;
    void update_channel_voices(int8_t channel, void(impl::*func)(opl_voice&));
    void update_patch(opl_voice& voice, const opl_patch* new_patch, uint8_t num_voice = 0);
    void update_volume(opl_voice& voice);
    void update_panning(opl_voice& voice);
    void update_frequency(opl_voice& voice);
    void silence_voice(opl_voice& voice);

    std::vector<ymfm::ymf262*> m_opl3;
    unsigned m_num_chips;
    opl_midi_synth::chip_type m_chip_type;

    bool m_stereo;
    sample_rate_t m_sample_rate;
    double m_sample_gain;
    double m_sample_step;
    double m_sample_pos;
    uint32_t m_samples_left;
    ymfm::ymf262::output_data m_output;
    std::vector<std::queue<ymfm::ymf262::output_data>> m_sample_fifo;

    int32_t m_last_out[2] = {0};
    double m_hp_filter_freq, m_hp_filter_coef;
    int32_t m_hp_last_in[2] = {0}, m_hp_last_out[2] = {0};
    float m_hp_last_in_f[2] = {0}, m_hp_last_out_f[2] = {0};

    bool m_looping;
    bool m_time_passed;

    midi_channel m_channels[16];
    std::vector<opl_voice> m_voices;
    opl_midi_synth::midi_type m_midi_type;

    midi_sequence_impl* m_sequence;
    opl_patch_set m_patches;
    opl_midi_synth* m_parent; // Reference to the public interface
};

} // namespace musac

#endif // MUSAC_OPL_MIDI_SYNTH_IMPL_HH