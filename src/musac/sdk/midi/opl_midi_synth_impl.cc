#include "opl_midi_synth_impl.hh"
#include <musac/sdk/midi/midi_sequence.hh>
#include <musac/sdk/midi/opl_patches.hh>
#include "sequence.h"
#include <cmath>
#include <cstring>

namespace musac {
	static const unsigned voice_num[18] = {
		0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
		0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108
	};

	static const unsigned oper_num[18] = {
		0x0, 0x1, 0x2, 0x8, 0x9, 0xA, 0x10, 0x11, 0x12,
		0x100, 0x101, 0x102, 0x108, 0x109, 0x10A, 0x110, 0x111, 0x112
	};

	// ----------------------------------------------------------------------------
	opl_midi_synth::impl::impl(int num_chips, opl_midi_synth::chip_type type, opl_midi_synth* parent)
		: ymfm::ymfm_interface(), m_parent(parent)
	{
		m_chip_type = type;
		if (type == opl_midi_synth::chip_opl3)
		{
			m_num_chips = num_chips;
			m_voices.resize(num_chips * 18);
			m_stereo = true;
		}
		else
		{
			// simulate two OPL2 on one OPL3, etc
			m_num_chips = (num_chips + 1) / 2;
			m_voices.resize(num_chips * 9);
			m_stereo = false;
		}

		m_opl3.resize(m_num_chips);
		for (auto& opl : m_opl3)
			opl = new ymfm::ymf262(*this);
		m_sample_fifo.resize(m_num_chips);

		m_sequence = nullptr;

		m_sample_pos = 0.0;
		m_samples_left = 0;
		m_hp_filter_freq = 5.0; // 5Hz default to reduce DC offset
		set_sample_rate(44100); // setup both sample step and filter coefficients
		set_gain(1.0);

		m_looping = false;

		reset();
	}

	// ----------------------------------------------------------------------------
	opl_midi_synth::impl::~impl()
	{
		for (auto& opl : m_opl3)
			delete opl;
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::set_sample_rate(sample_rate_t rate)
	{
		uint32_t rateOPL = m_opl3[0]->sample_rate(master_clock);
		m_sample_step = (double)rate / rateOPL;
		m_sample_rate = rate;

		set_filter(m_hp_filter_freq);
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::set_gain(double gain)
	{
		m_sample_gain = gain;
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::set_filter(double cutoff)
	{
		m_hp_filter_freq = cutoff;

		if (m_hp_filter_freq <= 0.0)
		{
			m_hp_filter_coef = 1.0;
		}
		else
		{
			static const double pi = 3.14159265358979323846;
			m_hp_filter_coef = 1.0 / ((2 * pi * cutoff) / m_sample_rate + 1);
		}
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::set_stereo(bool on)
	{
		if (m_chip_type == opl_midi_synth::chip_opl3)
		{
			m_stereo = on;
			update_channel_voices(-1, &opl_midi_synth::impl::update_panning);
		}
	}

	// ----------------------------------------------------------------------------
	bool opl_midi_synth::impl::load_sequence(const char* path)
	{
		auto seq = midi_sequence_impl::load(path);
		m_sequence = seq.release();
		
		return m_sequence != nullptr;
	}

	// ----------------------------------------------------------------------------
	bool opl_midi_synth::impl::load_sequence(FILE *file, int offset, size_t size)
	{
		auto seq = midi_sequence_impl::load(file, offset, size);
		m_sequence = seq.release();
		
		return m_sequence != nullptr;
	}

	bool opl_midi_synth::impl::load_sequence(musac::io_stream* file, int offset, size_t size) {
		auto seq = midi_sequence_impl::load(file, offset, size);
		m_sequence = seq.release();
		
		return m_sequence != nullptr;
	}

	// ----------------------------------------------------------------------------
	bool opl_midi_synth::impl::load_sequence(const uint8_t *data, size_t size)
	{
		auto seq = midi_sequence_impl::load(data, size);
		m_sequence = seq.release();
		
		return m_sequence != nullptr;
	}

	// ----------------------------------------------------------------------------
	bool opl_midi_synth::impl::load_patches(const char* path)
	{
		return opl_patch_loader::load(m_patches, path);
	}

	// ----------------------------------------------------------------------------
	bool opl_midi_synth::impl::load_patches(FILE *file, int offset, size_t size)
	{
		return opl_patch_loader::load(m_patches, file, offset, size);
	}

	// ----------------------------------------------------------------------------
	bool opl_midi_synth::impl::load_patches(const uint8_t *data, size_t size)
	{
		return opl_patch_loader::load(m_patches, data, size);
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::generate(float *data, unsigned num_samples)
	{
		unsigned samp = 0;

		while (samp < num_samples * 2)
		{
			update_midi();

			float samples[2];
			samples[0] = m_output.data[0] / 32767.0;
			samples[1] = m_output.data[1] / 32767.0;

			while (m_sample_pos >= 1.0 && samp < num_samples * 2)
			{
				data[samp]   = samples[0];
				data[samp+1] = samples[1];

				if (m_hp_filter_coef < 1.0)
				{
					for (int i = 0; i < 2; i++)
					{
						const float lastIn = m_hp_last_in_f[i];
						m_hp_last_in_f[i] = data[samp+i];

						m_hp_last_out_f[i] = m_hp_filter_coef * (m_hp_last_out_f[i] + data[samp+i] - lastIn);
						data[samp+i] = m_hp_last_out_f[i];
					}
				}

				samp += 2;
				m_sample_pos -= 1.0;
				if (m_samples_left)
					m_samples_left--;
			}
		}
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::generate(int16_t *data, unsigned num_samples)
	{
		unsigned samp = 0;

		while (samp < num_samples * 2)
		{
			update_midi();

			while (m_sample_pos >= 1.0 && samp < num_samples * 2)
			{
				if (m_hp_filter_coef < 1.0)
				{
					for (int i = 0; i < 2; i++)
					{
						const int32_t lastIn = m_hp_last_in[i];
						m_hp_last_in[i] = m_output.data[i];

						m_hp_last_out[i] = m_hp_filter_coef * (m_hp_last_out[i] + m_output.data[i] - lastIn);
						m_output.data[i] = m_hp_last_out[i];
					}
				}

				data[samp]   = ymfm::clamp(m_output.data[0], -32768, 32767);
				data[samp+1] = ymfm::clamp(m_output.data[1], -32768, 32767);

				samp += 2;
				m_sample_pos -= 1.0;
				if (m_samples_left)
					m_samples_left--;
			}
		}
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::update_midi()
	{
		while (!m_samples_left && m_sequence && !at_end())
		{
			// time to update midi playback
			m_samples_left = m_sequence->update(*m_parent);
			for (auto& voice : m_voices)
			{
				if (voice.duration < UINT_MAX)
					voice.duration++;
				voice.just_changed = false;
			}

			if (m_samples_left)
				m_time_passed = true;
		}

		if (m_sample_pos >= 1.0)
		{
			return; // existing output still waiting to be consumed
		}

		m_output.data[0] = m_last_out[0];
		m_output.data[1] = m_last_out[1];

		while (m_sample_pos < 1.0)
		{
			ymfm::ymf262::output_data output;
			int32_t samples[2] = {0};

			for (unsigned i = 0; i < m_num_chips; i++)
			{
				if (m_sample_fifo[i].empty())
				{
					m_opl3[i]->generate(&output);
				}
				else
				{
					output = m_sample_fifo[i].front();
					m_sample_fifo[i].pop();
				}

				samples[0] += output.data[0];
				samples[1] += output.data[1];
			}

			m_sample_pos += m_sample_step;

			if (m_sample_pos <= 1.0 || m_sample_step > 1.0)
			{
				// full input sample (if downsampling), or always (if upsampling)
				m_output.data[0] += samples[0];
				m_output.data[1] += samples[1];
				m_last_out[0] = m_last_out[1] = 0;
			}
			else
			{
				// partial input sample (if downsampling):
				// apply a fraction of the sample value now and save the rest for later
				// based on how far past the output sample point we are
				const double remainder = (m_sample_pos - (int)m_sample_pos) / m_sample_step;
				m_output.data[0] += samples[0] * (1 - remainder);
				m_output.data[1] += samples[1] * (1 - remainder);
				m_last_out[0] = samples[0] * remainder;
				m_last_out[1] = samples[1] * remainder;
			}
		}

		// apply gain and use sample rate in/out ratio to scale all accumulated samples
		const double step = std::min(m_sample_step, 1.0);
		m_output.data[0] *= m_sample_gain * step;
		m_output.data[1] *= m_sample_gain * step;
	}

	// ----------------------------------------------------------------------------

	// ----------------------------------------------------------------------------
	bool opl_midi_synth::impl::at_end() const
	{
		// rewind song at end only if looping is enabled
		// AND if the song played for at least one sample,
		// otherwise just leave it at the end
		if (m_looping && m_time_passed)
			return false;
		if (m_sequence)
			return m_sequence->at_end();
		return true;
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::set_song_num(unsigned num)
	{
		if (m_sequence) {
			m_sequence->set_song_num(num);
		}
		reset();
	}

	// ----------------------------------------------------------------------------
	unsigned opl_midi_synth::impl::num_songs() const
	{
		if (m_sequence)
			return m_sequence->num_songs();
		return 0;
	}

	// ----------------------------------------------------------------------------
	unsigned opl_midi_synth::impl::song_num() const
	{
		if (m_sequence)
			return m_sequence->song_num();
		return 0;
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::reset()
	{
		for (int i = 0; i < m_opl3.size(); i++)
		{
			m_opl3[i]->reset();
			// enable OPL3 stuff
			write(i, REG_NEW, 1);
		}

		// reset MIDI channel and OPL voice status
		m_midi_type = opl_midi_synth::general_midi;
		for (int i = 0; i < 16; i++)
		{
			m_channels[i] = midi_channel();
			m_channels[i].num = i;
		}
		m_channels[9].percussion = true;

		for (int i = 0; i < m_voices.size(); i++)
		{
			m_voices[i] = opl_voice();
			m_voices[i].chip = i / 18;
			m_voices[i].num = voice_num[i % 18];
			m_voices[i].op = oper_num[i % 18];

			// configure 4op voices (OPL3 mode only)
			if (m_chip_type != opl_midi_synth::chip_opl3) continue;
			switch (i % 9)
			{
				case 0: case 1: case 2:
					m_voices[i].four_op_primary = true;
				m_voices[i].four_op_other = &m_voices[i+3];
				break;
				case 3: case 4: case 5:
					m_voices[i].four_op_primary = false;
				m_voices[i].four_op_other = &m_voices[i-3];
				break;
				default:
					m_voices[i].four_op_primary = false;
				m_voices[i].four_op_other = nullptr;
				break;
			}
		}

		if (m_sequence) {
			m_sequence->reset();
		}
		m_samples_left = 0;
		m_time_passed = 0;
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::run_samples(int chip, unsigned count)
	{
		// add some delay between register writes where needed
		// (i.e. when forcing a voice off, changing 4op flags, etc.)
		while (count--)
		{
			ymfm::ymf262::output_data output;
			m_opl3[chip]->generate(&output);
			m_sample_fifo[chip].push(output);
		}
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::write(int chip, uint16_t addr, uint8_t data)
	{
		//	if (addr != 0x104)
		if (addr < 0x100)
			m_opl3[chip]->write_address((uint8_t)addr);
		else
			m_opl3[chip]->write_address_hi((uint8_t)addr);
		m_opl3[chip]->write_data(data);
	}

	// ----------------------------------------------------------------------------
	opl_voice* opl_midi_synth::impl::find_voice(uint8_t channel, const opl_patch *patch, uint8_t note)
	{
		opl_voice *found = nullptr;
		uint32_t duration = 0;

		// try to find the "oldest" voice, prioritizing released notes
		// (or voices that haven't ever been used yet)
		for (auto& voice : m_voices)
		{
			if (use_four_op(patch) && !voice.four_op_primary)
				continue;

			if (!voice.channel)
				return &voice;

			if (!voice.on && !voice.just_changed)
			{
				if (voice.channel->num == channel && voice.note == note
					&& voice.duration < UINT_MAX)
				{
					// found an old voice that was using the same note and patch
					// don't immediately use it, but make it a high priority candidate for later
					// (to help avoid pop/click artifacts when retriggering a recently off note)
					silence_voice(voice);
					if (use_four_op(voice.patch) && voice.four_op_other)
						silence_voice(*voice.four_op_other);
				}
				else if (voice.duration > duration)
				{
					found = &voice;
					duration = voice.duration;
				}
			}
		}

		if (found) return found;
		// if we didn't find one yet, just try to find an old one
		// using the same patch, even if it should still be playing.
		for (auto& voice : m_voices)
		{
			if (use_four_op(patch) && !voice.four_op_primary)
				continue;

			if (voice.patch == patch && voice.duration > duration)
			{
				found = &voice;
				duration = voice.duration;
			}
		}

		if (found) return found;
		// last resort - just find any old voice at all

		for (auto& voice : m_voices)
		{
			if (use_four_op(patch) && !voice.four_op_primary)
				continue;
			// don't let a 2op instrument steal an active voice from a 4op one
			if (!use_four_op(patch) && voice.on && use_four_op(voice.patch))
				continue;

			if (voice.duration > duration)
			{
				found = &voice;
				duration = voice.duration;
			}
		}

		return found;
	}

	// ----------------------------------------------------------------------------
	opl_voice* opl_midi_synth::impl::find_voice(uint8_t channel, uint8_t note, bool just_changed)
	{
		channel &= 15;
		for (auto& voice : m_voices)
		{
			if (voice.on
				&& voice.just_changed == just_changed
				&& voice.channel == &m_channels[channel]
				&& voice.note == note)
			{
				return &voice;
			}
		}

		return nullptr;
	}

	// ----------------------------------------------------------------------------
	const opl_patch* opl_midi_synth::impl::find_patch(uint8_t channel, uint8_t note) const
	{
		uint16_t key;
		const midi_channel &ch = m_channels[channel & 15];

		if (ch.percussion)
			key = 0x80 | note | (ch.patch_num << 8);
		else
			key = ch.patch_num | (ch.bank << 8);

		// if this patch+bank combo doesn't exist, default to bank 0
		if (!m_patches.count(key))
			key &= 0x00ff;
		// if patch still doesn't exist in bank 0, use patch 0 (or drum note 0)
		if (!m_patches.count(key))
			key &= 0x0080;
		// if that somehow still doesn't exist, forget it
		if (!m_patches.count(key))
			return nullptr;

		return &m_patches.at(key);
	}

	// ----------------------------------------------------------------------------
	bool opl_midi_synth::impl::use_four_op(const opl_patch *patch) const
	{
		if (m_chip_type == opl_midi_synth::chip_opl3)
			return patch->four_op;
		return false;
	}

	// ----------------------------------------------------------------------------
	std::pair<bool, bool> opl_midi_synth::impl::active_carriers(const opl_voice& voice) const
	{
		bool scale[2] = {0};
		const auto patch_voice = voice.voice_patch;

		if (!patch_voice)
		{
			scale[0] = scale[1] = false;
		}
		else if (!use_four_op(voice.patch))
		{
			// 2op FM (0): scale op 2 only
			// 2op AM (1): scale op 1 and 2
			scale[0] = (patch_voice->conn & 1);
			scale[1] = true;
		}
		else if (voice.four_op_primary)
		{
			// 4op FM+FM (0, 0): don't scale op 1 or 2
			// 4op AM+FM (1, 0): scale op 1 only
			// 4op FM+AM (0, 1): scale op 2 only
			// 4op AM+AM (1, 1): scale op 1 only
			scale[0] = (voice.patch->voice[0].conn & 1);
			scale[1] = (voice.patch->voice[1].conn & 1) && !scale[0];
		}
		else
		{
			// 4op FM+FM (0, 0): scale op 4 only
			// 4op AM+FM (1, 0): scale op 4 only
			// 4op FM+AM (0, 1): scale op 4 only
			// 4op AM+AM (1, 1): scale op 3 and 4
			scale[0] = (voice.patch->voice[0].conn & 1)
					&& (voice.patch->voice[1].conn & 1);
			scale[1] = true;
		}

		return std::make_pair(scale[0], scale[1]);
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::update_channel_voices(int8_t channel, void(opl_midi_synth::impl::*func)(opl_voice&))
	{
		for (auto& voice : m_voices)
		{
			if ((channel < 0) || (voice.channel == &m_channels[channel & 15]))
				(this->*func)(voice);
		}
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::update_patch(opl_voice& voice, const opl_patch *newPatch, uint8_t numVoice)
	{
		// assign the MIDI channel's current patch (or the current drum patch) to this voice

		const patch_voice& patch_voice = newPatch->voice[numVoice];

		if (voice.voice_patch != &patch_voice)
		{
			bool oldFourOp = voice.patch ? use_four_op(voice.patch) : false;

			voice.patch = newPatch;
			voice.voice_patch = &patch_voice;

			// update enable status for 4op channels on this chip
			if (use_four_op(newPatch) != oldFourOp)
			{
				// if going from part of a 4op patch to a 2op one, kill the other one
				opl_voice *other = voice.four_op_other;
				if (other && other->patch
					&& use_four_op(other->patch) && !use_four_op(newPatch))
				{
					silence_voice(*other);
				}

				uint8_t enable = 0x00;
				uint8_t bit = 0x01;
				for (unsigned i = voice.chip * 18; i < voice.chip * 18 + 18; i++)
				{
					if (m_voices[i].four_op_primary)
					{
						if (m_voices[i].patch && use_four_op(m_voices[i].patch))
							enable |= bit;
						bit <<= 1;
					}
				}

				write(voice.chip, REG_4OP, enable);
				//	run_samples(voice.chip, 1);
			}

			// kill an existing voice, then send the chip far enough forward in time to let the envelope die off
			// (ROTT: fixes nasty reverse cymbal noises in spray.mid
			//        without disrupting note timing too much for the staccato drums in fanfare2.mid)
			silence_voice(voice);
			run_samples(voice.chip, 48);

			// 0x20: vibrato, sustain, multiplier
			write(voice.chip, REG_OP_MODE + voice.op,     patch_voice.op_mode[0]);
			write(voice.chip, REG_OP_MODE + voice.op + 3, patch_voice.op_mode[1]);
			// 0x60: attack/decay
			write(voice.chip, REG_OP_AD + voice.op,     patch_voice.op_ad[0]);
			write(voice.chip, REG_OP_AD + voice.op + 3, patch_voice.op_ad[1]);
			// 0xe0: waveform
			if (m_chip_type == opl_midi_synth::chip_opl2)
			{
				write(voice.chip, REG_OP_WAVEFORM + voice.op,     patch_voice.op_wave[0] & 3);
				write(voice.chip, REG_OP_WAVEFORM + voice.op + 3, patch_voice.op_wave[1] & 3);
			}
			else if (m_chip_type == opl_midi_synth::chip_opl3)
			{
				write(voice.chip, REG_OP_WAVEFORM + voice.op,     patch_voice.op_wave[0]);
				write(voice.chip, REG_OP_WAVEFORM + voice.op + 3, patch_voice.op_wave[1]);
			}
		}

		// 0x80: sustain/release
		// update even for the same patch in case silence_voice was called from somewhere else on this voice
		write(voice.chip, REG_OP_SR + voice.op,     patch_voice.op_sr[0]);
		write(voice.chip, REG_OP_SR + voice.op + 3, patch_voice.op_sr[1]);
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::update_volume(opl_voice& voice)
	{
		// lookup table shamelessly stolen from Nuke.YKT
		static const uint8_t opl_volume_map[32] =
		{
			80, 63, 40, 36, 32, 28, 23, 21,
			19, 17, 15, 14, 13, 12, 11, 10,
			 9,  8,  7,  6,  5,  5,  4,  4,
			 3,  3,  2,  2,  1,  1,  0,  0
		};

		if (!voice.patch || !voice.channel) return;

		uint8_t atten = opl_volume_map[(voice.velocity * voice.channel->volume) >> 9];
		uint8_t level;

		const auto patch_voice = voice.voice_patch;
		const auto scale = active_carriers(voice);

		// 0x40: key scale / volume
		if (scale.first)
			level = std::min(0x3f, patch_voice->op_level[0] + atten);
		else
			level = patch_voice->op_level[0];
		write(voice.chip, REG_OP_LEVEL + voice.op,     level | patch_voice->op_ksr[0]);

		if (scale.second)
			level = std::min(0x3f, patch_voice->op_level[1] + atten);
		else
			level = patch_voice->op_level[1];
		write(voice.chip, REG_OP_LEVEL + voice.op + 3, level | patch_voice->op_ksr[1]);
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::update_panning(opl_voice& voice)
	{
		if (!voice.patch || !voice.channel) return;

		// 0xc0: output/feedback/mode
		uint8_t pan = 0x30;
		if (m_stereo)
		{
			if (voice.channel->pan < 32)
				pan = 0x10;
			else if (voice.channel->pan >= 96)
				pan = 0x20;
		}

		write(voice.chip, REG_VOICE_CNT + voice.num, voice.voice_patch->conn | pan);
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::update_frequency(opl_voice& voice)
	{
		static const uint16_t noteFreq[12] = {
			// calculated from A440
			345, 365, 387, 410, 435, 460, 488, 517, 547, 580, 615, 651
		};

		if (!voice.patch || !voice.channel) return;
		if (use_four_op(voice.patch) && !voice.four_op_primary) return;

		int note = (!voice.channel->percussion ? voice.note : voice.patch->fixed_note)
				 + voice.voice_patch->tune;

		int octave = note / 12;
		note %= 12;

		// calculate base frequency (and apply pitch bend / patch detune)
		unsigned freq = (note >= 0) ? noteFreq[note] : (noteFreq[note + 12] >> 1);
		if (octave < 0)
			freq >>= -octave;
		else if (octave > 0)
			freq <<= octave;

		freq *= voice.channel->pitch * voice.voice_patch->finetune;

		// convert the calculated frequency back to a block and F-number
		octave = 0;
		while (freq > 0x3ff)
		{
			freq >>= 1;
			octave++;
		}
		octave = std::min(7, octave);
		voice.freq = freq | (octave << 10);

		write(voice.chip, REG_VOICE_FREQL + voice.num, voice.freq & 0xff);
		write(voice.chip, REG_VOICE_FREQH + voice.num, (voice.freq >> 8) | (voice.on ? (1 << 5) : 0));
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::silence_voice(opl_voice& voice)
	{
		voice.on = false;
		voice.just_changed = true;
		voice.duration = UINT_MAX;

		write(voice.chip, REG_OP_SR + voice.op,     0xff);
		write(voice.chip, REG_OP_SR + voice.op + 3, 0xff);
		write(voice.chip, REG_VOICE_FREQH + voice.num, voice.freq >> 8);
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::midi_event(uint8_t status, uint8_t data0, uint8_t data1)
	{
		uint8_t channel = status & 15;
		int16_t pitch;

		switch (status >> 4)
		{
			case 8: // note off (ignore velocity)
				midi_note_off(channel, data0);
			break;

			case 9: // note on
				midi_note_on(channel, data0, data1);
			break;

			case 10: // polyphonic pressure (ignored)
				break;

			case 11: // controller change
				midi_control_change(channel, data0, data1);
			break;

			case 12: // program change
				midi_program_change(channel, data0);
			break;

			case 13: // channel pressure (ignored)
				break;

			case 14: // pitch bend
				pitch = (int16_t)(data0 | (data1 << 7)) - 8192;
			midi_pitch_control(channel, pitch / 8192.0);
			break;
		}
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity)
	{
		note &= 0x7f;
		velocity &= 0x7f;

		// if we just now turned this same note on, don't do it again
		if (find_voice(channel, note, true))
			return;

		if (!velocity)
			return midi_note_off(channel, note);

		const opl_patch *newPatch = find_patch(channel, note);
		if (!newPatch) return;

		const int numVoices = ((use_four_op(newPatch) || newPatch->dual_two_op) ? 2 : 1);

		opl_voice *voice = nullptr;
		for (int i = 0; i < numVoices; i++)
		{
			if (voice && use_four_op(newPatch) && voice->four_op_other)
				voice = voice->four_op_other;
			else
				voice = find_voice(channel, newPatch, note);
			if (!voice) continue; // ??

			update_patch(*voice, newPatch, i);

			// update the note parameters for this voice
			voice->channel = &m_channels[channel & 15];
			voice->on = voice->just_changed = true;
			voice->note = note;
			voice->velocity = ymfm::clamp((int)velocity + newPatch->velocity, 0, 127);
			voice->duration = 0;

			update_volume(*voice);
			update_panning(*voice);

			// for 4op instruments, don't key on until we've written both voices...
			if (!use_four_op(newPatch))
			{
				update_frequency(*voice);
			}
			else if (i > 0)
			{
				update_frequency(*voice->four_op_other);
			}
		}
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::midi_note_off(uint8_t channel, uint8_t note)
	{
		note &= 0x7f;

		opl_voice *voice;
		while ((voice = find_voice(channel, note)) != nullptr)
		{
			voice->just_changed = voice->on;
			voice->on = false;

			write(voice->chip, REG_VOICE_FREQH + voice->num, voice->freq >> 8);
		}
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::midi_pitch_control(uint8_t channel, double pitch)
	{
		midi_channel& ch = m_channels[channel & 15];

		ch.base_pitch = pitch;
		ch.pitch = midi_calc_bend(pitch * ch.bend_range);
		update_channel_voices(channel, &opl_midi_synth::impl::update_frequency);
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::midi_program_change(uint8_t channel, uint8_t patch_num)
	{
		m_channels[channel & 15].patch_num = patch_num & 0x7f;
		// patch change will take effect on the next note for this channel
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::midi_control_change(uint8_t channel, uint8_t control, uint8_t value)
	{
		channel &= 15;
		control &= 0x7f;
		value   &= 0x7f;

		midi_channel& ch = m_channels[channel];

		switch (control)
		{
			case 0:
				if (m_midi_type == opl_midi_synth::roland_gs)
					ch.bank = value;
				else if (m_midi_type == opl_midi_synth::yamaha_xg)
					ch.percussion = (value == 0x7f);
			break;

			case 6:
				if (ch.rpn == 0)
				{
					ch.bend_range = value;
					midi_pitch_control(channel, ch.base_pitch);
				}
			break;

			case 7:
				ch.volume = value;
			update_channel_voices(channel, &opl_midi_synth::impl::update_volume);
			break;

			case 10:
				ch.pan = value;
			if (m_stereo)
				update_channel_voices(channel, &opl_midi_synth::impl::update_panning);
			break;

			case 32:
				if (m_midi_type == opl_midi_synth::yamaha_xg || m_midi_type == opl_midi_synth::general_midi2)
					ch.bank = value;
			break;

			case 98:
			case 99:
				ch.rpn = 0x3fff;
			break;

			case 100:
				ch.rpn &= 0x3f80;
			ch.rpn |= value;
			break;

			case 101:
				ch.rpn &= 0x7f;
			ch.rpn |= (value << 7);
			break;

		}
	}

	// ----------------------------------------------------------------------------
	void opl_midi_synth::impl::midi_sysex(const uint8_t *data, uint32_t length)
	{
		if (length > 0 && data[0] == 0xF0)
		{
			data++;
			length--;
		}

		if (length == 0)
			return;

		if (data[0] == 0x7e) // universal non-realtime
		{
			if (length == 5 && data[1] == 0x7f && data[2] == 0x09)
			{
				if (data[3] == 0x01)
					m_midi_type = opl_midi_synth::general_midi;
				else if (data[3] == 0x03)
					m_midi_type = opl_midi_synth::general_midi2;
			}
		}
		else if (data[0] == 0x41 && length >= 10 // Roland
				 && data[2] == 0x42 && data[3] == 0x12)
		{
			// if we received one of these, assume GS mode
			// (some MIDIs seem to e.g. send drum map messages without a GS reset)
			m_midi_type = opl_midi_synth::roland_gs;

			uint32_t address = (data[4] << 16) | (data[5] << 8) | data[6];
			// for single part parameters, map "part number" to channel number
			// (using the default mapping)
			uint8_t channel = (address & 0xf00) >> 8;
			if (channel == 0)
				channel = 9;
			else if (channel <= 9)
				channel--;

			// Roland GS part parameters
			if ((address & 0xfff0ff) == 0x401015) // set drum map
				m_channels[channel].percussion = (data[7] != 0x00);
		}
		else if (length >= 8 && !memcmp(data, "\x43\x10\x4c\x00\x00\x7e\x00\xf7", 8)) // Yamaha
		{
			m_midi_type = opl_midi_synth::yamaha_xg;
		}
	}

	// ----------------------------------------------------------------------------
	double opl_midi_synth::impl::midi_calc_bend(double semitones)
	{
		return pow(2, semitones / 12.0);
	}
	
	// ----------------------------------------------------------------------------
	uint64_t opl_midi_synth::impl::calculate_duration_samples()
	{
		if (!m_sequence) {
			return 0;
		}
		
		// Save current state
		auto saved_sequence = m_sequence;
		uint32_t saved_samples_left = m_samples_left;
		bool saved_at_end = at_end();
		
		// Reset and fast-forward through entire sequence
		m_sequence->reset();
		uint64_t total_samples = 0;
		
		// Create a dummy synth for fast-forward (no audio generation)
		// We just need to accumulate sample counts
		while (!m_sequence->at_end()) {
			uint32_t samples = m_sequence->update(*m_parent);
			if (samples == 0 && m_sequence->at_end()) {
				break;
			}
			total_samples += samples;
			
			// Safety check to prevent infinite loops
			if (total_samples > 44100ULL * 60 * 60) { // 1 hour max
				break;
			}
		}
		
		// Restore state
		m_sequence->reset();
		if (saved_at_end) {
			// If we were at the end, stay there
			while (!m_sequence->at_end()) {
				m_sequence->update(*m_parent);
			}
		}
		m_samples_left = saved_samples_left;
		
		return total_samples;
	}
	
	// ----------------------------------------------------------------------------
	bool opl_midi_synth::impl::seek_to_sample(uint64_t sample_pos)
	{
		if (!m_sequence) {
			return false;
		}
		
		// Reset to beginning
		reset();
		
		// Fast-forward to target position
		uint64_t current_pos = 0;
		
		while (current_pos < sample_pos && !m_sequence->at_end()) {
			uint32_t samples = m_sequence->update(*m_parent);
			if (samples == 0 && m_sequence->at_end()) {
				break;
			}
			
			if (current_pos + samples > sample_pos) {
				// We've reached the target position
				// Set remaining samples in current event
				m_samples_left = samples - (sample_pos - current_pos);
				break;
			}
			
			current_pos += samples;
		}
		
		return true;
	}
}