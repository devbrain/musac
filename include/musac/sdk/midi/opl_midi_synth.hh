#ifndef MUSAC_OPL_MIDI_SYNTH_HH
#define MUSAC_OPL_MIDI_SYNTH_HH

#include <musac/sdk/types.hh>
#include <musac/sdk/export_musac_sdk.h>
#include <memory>
#include <string>
#include <cstdint>

namespace musac {
    
class io_stream;

class MUSAC_SDK_EXPORT opl_midi_synth {
public:
    enum midi_type {
        general_midi,
        roland_gs,
        yamaha_xg,
        general_midi2
    };

    enum chip_type {
        chip_opl,
        chip_opl2,
        chip_opl3
    };

    opl_midi_synth(int num_chips = 1, chip_type type = chip_opl3);
    ~opl_midi_synth();

    void set_loop(bool loop);
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
    
    // Duration and seeking support
    uint64_t calculate_duration_samples();
    bool seek_to_sample(uint64_t sample_pos);

    void midi_event(uint8_t status, uint8_t data0, uint8_t data1 = 0);
    void midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void midi_note_off(uint8_t channel, uint8_t note);
    void midi_pitch_control(uint8_t channel, double pitch);
    void midi_program_change(uint8_t channel, uint8_t patch_num);
    void midi_control_change(uint8_t channel, uint8_t control, uint8_t value);
    void midi_sysex(const uint8_t* data, uint32_t length);

    static double midi_calc_bend(double semitones);

    sample_rate_t sample_rate() const;
    chip_type get_chip_type() const;
    bool stereo() const;
    const std::string& patch_name(uint8_t num) const;

private:
    class impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace musac

#endif // MUSAC_OPL_MIDI_SYNTH_HH