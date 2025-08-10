#include <musac/sdk/midi/opl_patches.hh>
#include <musac/sdk/midi/opl_midi_synth.hh>
#include <musac/sdk/io_stream.hh>
#include <cstdio>
#include <cstring>
#include <vector>

namespace musac {
	// ----------------------------------------------------------------------------
	bool opl_patch_loader::load(opl_patch_set& patches, const char *path)
	{
		FILE *file = fopen(path, "rb");
		if (!file) return false;

		bool ok = load(patches, file);

		fclose(file);

		return ok;
	}

	// ----------------------------------------------------------------------------
	bool opl_patch_loader::load(opl_patch_set& patches, FILE *file, int offset, size_t size)
	{
		if (!size)
		{
			fseek(file, 0, SEEK_END);
			if (ftell(file) < 0)
				return false;
			size = ftell(file) - offset;
		}

		fseek(file, offset, SEEK_SET);
		std::vector<uint8_t> data(size);
		if (fread(data.data(), 1, size, file) != size)
			return false;

		return load(patches, data.data(), size);
	}

	bool opl_patch_loader::load(opl_patch_set& patches, io_stream* file, int offset, size_t size) {
		if (!size)
		{
			file->seek(offset, seek_origin::end);
			auto p = file->tell();
			if (p < 0)
				return false;
			size = p - offset;
		}

		file->seek(offset, seek_origin::set);
		std::vector<uint8_t> data(size);

		if (file->read( data.data(), size) != size)
			return false;

		return load(patches, data.data(), size);
	}

	// ----------------------------------------------------------------------------
	bool opl_patch_loader::load(opl_patch_set& patches, const uint8_t *data, size_t size)
	{
		return load_wopl(patches, data, size)
			|| load_op2(patches, data, size)
			|| load_ail(patches, data, size)
			|| load_tmb(patches, data, size);
	}

	// ----------------------------------------------------------------------------
	bool opl_patch_loader::load_wopl(opl_patch_set& patches, const uint8_t *data, size_t size)
	{
		if (size < 19)
			return false;
		if (std::memcmp(data, "WOPL3-BANK", 10) != 0)
			return false;

		// mixed endianness? why???
		uint16_t version   = data[11] | (data[12] << 8);
		uint16_t num_melody = (data[13] << 8) | data[14];
		uint16_t num_perc   = (data[15] << 8) | data[16];

		if (version > 3)
			return false;

		// currently not supported: global LFO flags, volume model options

		const uint32_t bank_offset  = 19;
		const uint32_t patch_offset = bank_offset + 34 * (num_melody + num_perc);

		const unsigned inst_size = (version >= 3) ? 66 : 62;
		const unsigned bank_info_size = (version >= 2) ? 34 : 0;

		if (size < (num_melody + num_perc) * (128 * inst_size + bank_info_size))
			return false;

		for (unsigned i = 0; i < 128 * (num_melody + num_perc); i++)
		{
			const uint8_t *bytes;
			unsigned key = i & 0x7f;
			if (version >= 2)
			{
				const unsigned bank = i >> 7;
				bytes = data + bank_offset + 34 * bank;

				if (bank >= num_melody) // percussion banks (use LSB)
					key |= (bytes[32] << 8) | 0x80;
				else if (bytes[32]) // bank LSB set (XG)
					key |= (bytes[32] << 8);
				else if (bytes[33]) // bank MSB set (GS)
					key |= (bytes[33] << 8);
			}

			bytes = data + patch_offset + inst_size * i;

			// ignore other data for this patch if it's a blank instrument
			// *or* if one of the rhythm mode bits is set (not supported here)
			if (bytes[39] & 0x3c)
				continue;

			opl_patch &patch = patches[key];
			// clear patch data
			patch = opl_patch();

			// patch names
			if (bytes[0])
				patch.name = std::string((const char*)bytes, 31);
			else
				patch.name = opl_patch::names[key & 0xff];

			// patch global settings
			patch.voice[0].tune     = (int8_t)bytes[33] - 12;
			patch.voice[1].tune     = (int8_t)bytes[35] - 12;
			patch.velocity          = (int8_t)bytes[36];
			patch.voice[1].finetune = opl_midi_synth::midi_calc_bend((int8_t)bytes[37] / 64.0);
			patch.fixed_note         = bytes[38];
			patch.four_op            = (bytes[39] & 3) == 1;
			patch.dual_two_op         = (bytes[39] & 3) == 3;
			patch.voice[0].conn = bytes[40];
			patch.voice[1].conn = bytes[41];

			// patch operator settings
			unsigned pos = 42;
			for (unsigned op = 0; op < 4; op++)
			{
				patch_voice &voice = patch.voice[op/2];

				const unsigned n = (op % 2) ^ 1;

				voice.op_mode[n]  = bytes[pos++];
				voice.op_ksr[n]   = bytes[pos]   & 0xc0;
				voice.op_level[n] = bytes[pos++] & 0x3f;
				voice.op_ad[n]    = bytes[pos++];
				voice.op_sr[n]    = bytes[pos++];
				voice.op_wave[n]  = bytes[pos++];
			}
		}

		return true;
	}

	// ----------------------------------------------------------------------------
	bool opl_patch_loader::load_op2(opl_patch_set& patches, const uint8_t *data, size_t size)
	{
		if (size < 175 * (36 + 32) + 8)
			return false;

		if (strncmp((const char*)data, "#OPL_II#", 8))
			return false;

		// read data for all patches (128 melodic + 47 percussion)
		for (int i = 0; i < 128+47; i++)
		{
			// patches 0-127 are melodic; the rest are for percussion notes 35 thru 81
			unsigned key = (i < 128) ? i : (i + 35);

			opl_patch &patch = patches[key];
			// clear patch data
			patch = opl_patch();

			// seek to patch data
			const uint8_t *bytes = data + (36*i) + 8;

			// read the common data for both 2op voices
			// flag bit 0 is "fixed pitch" (for drums), but it's seemingly only used for drum patches anyway, so ignore it?
			patch.dual_two_op = (bytes[0] & 4);
			// second voice detune
			patch.voice[1].finetune = opl_midi_synth::midi_calc_bend((int8_t)(bytes[2] - 128) / 64.0);

			patch.fixed_note = bytes[3];

			// read data for both 2op voices
			unsigned pos = 4;
			for (int j = 0; j < 2; j++)
			{
				patch_voice &voice = patch.voice[j];

				for (int op = 0; op < 2; op++)
				{
					// operator mode
					voice.op_mode[op] = bytes[pos++];
					// operator envelope
					voice.op_ad[op] = bytes[pos++];
					voice.op_sr[op] = bytes[pos++];
					// operator waveform
					voice.op_wave[op] = bytes[pos++];
					// KSR & output level
					voice.op_ksr[op] = bytes[pos++] & 0xc0;
					voice.op_level[op] = bytes[pos++] & 0x3f;

					// feedback/connection (first op only)
					if (op == 0)
						voice.conn = bytes[pos];
					pos++;
				}

				// midi note offset (int16_t, but only really need the LSB)
				voice.tune = (int8_t)bytes[pos];
				pos += 2;
			}

			// fix for some bugged DMX patches (e.g. Doom II electric snare)
			if (!(patch.voice[1].op_ad[0] | patch.voice[1].op_ad[1]))
				patch.dual_two_op = false;

			// seek to patch name
			bytes = data + (32*i) + (36*175) + 8;
			if (bytes[0])
				patch.name = std::string((const char*)bytes, 31);
			else
				patch.name = opl_patch::names[key];
		}

		return true;
	}

	// ----------------------------------------------------------------------------
	bool opl_patch_loader::load_ail(opl_patch_set& patches, const uint8_t *data, size_t size)
	{
		int index = 0;

		while (true)
		{
			if (size < index * 6)
				return false;

			const uint8_t *entry = data + (index * 6);
			uint16_t key;

			if (entry[0] == 0xff && entry[1] == 0xff)
				return true; // end of patches
			else if (entry[1] == 0x7f)
				key = entry[0] | 0x80;
			else
				key = (entry[0] | (entry[1] << 8)) & 0x7f7f;

			opl_patch &patch = patches[key];
			// clear patch data
			patch = opl_patch();
			patch.name = opl_patch::names[key & 0xff];

			uint32_t patchPos = entry[2] | (entry[3] << 8) | (entry[4] << 16) | (entry[5] << 24);
			if (size < patchPos)
				return false;

			const uint8_t *bytes = data + patchPos;

			if (size < patchPos + bytes[0])
				return false;
			else if (bytes[0] == 0x0e)
				patch.four_op = false;
			else if (bytes[0] == 0x19)
				patch.four_op = true;
			else
				return false;
			index++;

			patch.voice[0].tune = patch.voice[1].tune = (int8_t)bytes[2] - 12;
			patch.voice[0].conn = bytes[8] & 0x0f;
			patch.voice[1].conn = bytes[8] >> 7;

			unsigned pos = 3;
			for (int i = 0; i < (patch.four_op ? 2 : 1); i++)
			{
				patch_voice &voice = patch.voice[i];

				for (int op = 0; op < 2; op++)
				{
					// operator mode
					voice.op_mode[op] = bytes[pos++];
					// KSR & output level
					voice.op_ksr[op] = bytes[pos] & 0xc0;
					voice.op_level[op] = bytes[pos++] & 0x3f;
					// operator envelope
					voice.op_ad[op] = bytes[pos++];
					voice.op_sr[op] = bytes[pos++];
					// operator waveform
					voice.op_wave[op] = bytes[pos++];

					// already handled the feedback/connection byte
					if (op == 0)
						pos++;
				}
			}
		}
	}

	// ----------------------------------------------------------------------------
	bool opl_patch_loader::load_tmb(opl_patch_set& patches, const uint8_t *data, size_t size)
	{
		if (size < 256 * 13)
			return false;

		for (uint16_t key = 0; key < 256; key++)
		{
			opl_patch &patch = patches[key];
			// clear patch data
			patch = opl_patch();
			patch.name = opl_patch::names[key];

			const uint8_t *bytes = data + (key * 13);

			// since this format has no identifying info, we can only really reject it
			// if it has invalid values in a few spots
			if ((bytes[8] | bytes[9] | bytes[10]) & 0xf0)
				return false;

			patch_voice &voice = patch.voice[0];
			voice.op_mode[0]  = bytes[0];
			voice.op_mode[1]  = bytes[1];
			voice.op_ksr[0]   = bytes[2] & 0xc0;
			voice.op_level[0] = bytes[2] & 0x3f;
			voice.op_ksr[1]   = bytes[3] & 0xc0;
			voice.op_level[1] = bytes[3] & 0x3f;
			voice.op_ad[0]    = bytes[4];
			voice.op_ad[1]    = bytes[5];
			voice.op_sr[0]    = bytes[6];
			voice.op_sr[1]    = bytes[7];
			voice.op_wave[0]  = bytes[8];
			voice.op_wave[1]  = bytes[9];
			voice.conn        = bytes[10];
			voice.tune        = (int8_t)bytes[11] - 12;
			patch.velocity    = (int8_t)bytes[12];
		}

		return true;
	}
}