#include <musac/sdk/midi/opl_midi_synth.hh>
#include "opl_midi_synth_impl.hh"

namespace musac {

opl_midi_synth::opl_midi_synth(int num_chips, chip_type type)
    : m_impl(std::make_unique<impl>(num_chips, type, this)) {
}

opl_midi_synth::~opl_midi_synth() = default;

void opl_midi_synth::set_loop(bool loop) {
    m_impl->set_loop(loop);
}

void opl_midi_synth::set_sample_rate(sample_rate_t rate) {
    m_impl->set_sample_rate(rate);
}

void opl_midi_synth::set_gain(double gain) {
    m_impl->set_gain(gain);
}

void opl_midi_synth::set_filter(double cutoff) {
    m_impl->set_filter(cutoff);
}

void opl_midi_synth::set_stereo(bool on) {
    m_impl->set_stereo(on);
}

bool opl_midi_synth::load_sequence(const char* path) {
    return m_impl->load_sequence(path);
}

bool opl_midi_synth::load_sequence(FILE* file, int offset, size_t size) {
    return m_impl->load_sequence(file, offset, size);
}

bool opl_midi_synth::load_sequence(io_stream* file, int offset, size_t size) {
    return m_impl->load_sequence(file, offset, size);
}

bool opl_midi_synth::load_sequence(const uint8_t* data, size_t size) {
    return m_impl->load_sequence(data, size);
}

bool opl_midi_synth::load_patches(const char* path) {
    return m_impl->load_patches(path);
}

bool opl_midi_synth::load_patches(FILE* file, int offset, size_t size) {
    return m_impl->load_patches(file, offset, size);
}

bool opl_midi_synth::load_patches(const uint8_t* data, size_t size) {
    return m_impl->load_patches(data, size);
}

void opl_midi_synth::generate(float* data, unsigned num_samples) {
    m_impl->generate(data, num_samples);
}

void opl_midi_synth::generate(int16_t* data, unsigned num_samples) {
    m_impl->generate(data, num_samples);
}

void opl_midi_synth::reset() {
    m_impl->reset();
}

bool opl_midi_synth::at_end() const {
    return m_impl->at_end();
}

void opl_midi_synth::set_song_num(unsigned num) {
    m_impl->set_song_num(num);
}

unsigned opl_midi_synth::num_songs() const {
    return m_impl->num_songs();
}

unsigned opl_midi_synth::song_num() const {
    return m_impl->song_num();
}

void opl_midi_synth::midi_event(uint8_t status, uint8_t data0, uint8_t data1) {
    m_impl->midi_event(status, data0, data1);
}

void opl_midi_synth::midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    m_impl->midi_note_on(channel, note, velocity);
}

void opl_midi_synth::midi_note_off(uint8_t channel, uint8_t note) {
    m_impl->midi_note_off(channel, note);
}

void opl_midi_synth::midi_pitch_control(uint8_t channel, double pitch) {
    m_impl->midi_pitch_control(channel, pitch);
}

void opl_midi_synth::midi_program_change(uint8_t channel, uint8_t patch_num) {
    m_impl->midi_program_change(channel, patch_num);
}

void opl_midi_synth::midi_control_change(uint8_t channel, uint8_t control, uint8_t value) {
    m_impl->midi_control_change(channel, control, value);
}

void opl_midi_synth::midi_sysex(const uint8_t* data, uint32_t length) {
    m_impl->midi_sysex(data, length);
}

double opl_midi_synth::midi_calc_bend(double semitones) {
    return impl::midi_calc_bend(semitones);
}

sample_rate_t opl_midi_synth::sample_rate() const {
    return m_impl->sample_rate();
}

opl_midi_synth::chip_type opl_midi_synth::get_chip_type() const {
    return m_impl->chip_type();
}

bool opl_midi_synth::stereo() const {
    return m_impl->stereo();
}

const std::string& opl_midi_synth::patch_name(uint8_t num) const {
    return m_impl->patch_name(num);
}

uint64_t opl_midi_synth::calculate_duration_samples() {
    return m_impl->calculate_duration_samples();
}

bool opl_midi_synth::seek_to_sample(uint64_t sample_pos) {
    return m_impl->seek_to_sample(sample_pos);
}

} // namespace musac