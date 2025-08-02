
#include <cstdio>

#include "musac/codecs/seq/sequence.h"
#include "musac/codecs/seq/sequence_hmi.h"
#include "musac/codecs/seq/sequence_hmp.h"
#include "musac/codecs/seq/sequence_mid.h"
#include "musac/codecs/seq/sequence_mus.h"
#include "musac/codecs/seq/sequence_xmi.h"
namespace ymfmidi {
	// ----------------------------------------------------------------------------
	Sequence::~Sequence() {}

	// ----------------------------------------------------------------------------
	Sequence* Sequence::load(const char *path)
	{
		FILE *file = fopen(path, "rb");
		if (!file) return nullptr;

		Sequence *seq = load(file);

		fclose(file);
		return seq;
	}

	// ----------------------------------------------------------------------------
	Sequence* Sequence::load(FILE *file, int offset, size_t size)
	{
		if (!size)
		{
			fseek(file, 0, SEEK_END);
			if (ftell(file) < 0)
				return nullptr;
			size = ftell(file) - offset;
		}

		fseek(file, offset, SEEK_SET);
		std::vector<uint8_t> data(size);
		if (fread(data.data(), 1, size, file) != size)
			return nullptr;

		return load(data.data(), size);
	}

	Sequence* Sequence::load(musac::io_stream* file, int offset, size_t size) {
		if (!size)
		{
			file->seek( offset, musac::seek_origin::end);
			auto p = file->tell();
			if (p < 0)
				return nullptr;
			size = p - offset;
		}

		file->seek( offset, musac::seek_origin::set);
		std::vector<uint8_t> data(size);

		if (file->read( data.data(), size) != size)
			return nullptr;

		return load(data.data(), size);
	}

	// ----------------------------------------------------------------------------
	Sequence* Sequence::load(const uint8_t *data, size_t size)
	{
		Sequence *seq = nullptr;

		if (SequenceMUS::isValid(data, size))
			seq = new SequenceMUS();
		else if (SequenceMID::isValid(data, size))
			seq = new SequenceMID();
		else if (SequenceXMI::isValid(data, size))
			seq = new SequenceXMI();
		else if (SequenceHMI::isValid(data, size))
			seq = new SequenceHMI();
		else if (SequenceHMP::isValid(data, size))
			seq = new SequenceHMP();

		if (seq)
		{
			seq->read(data, size);
			seq->reset();
		}

		return seq;
	}
}