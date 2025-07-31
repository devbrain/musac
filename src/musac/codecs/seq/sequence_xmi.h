#ifndef __SEQUENCE_XMI_H
#define __SEQUENCE_XMI_H

#include "musac/codecs/seq/sequence_mid.h"

namespace ymfmidi {
	class SequenceXMI : public SequenceMID
	{
		public:
		SequenceXMI();
		~SequenceXMI() override;

		void setTimePerBeat(uint32_t usec) override;

		static bool isValid(const uint8_t *data, size_t size);

		private:
		void read(const uint8_t *data, size_t size) override;
		uint32_t readRootChunk(const uint8_t *data, size_t size);
	};
}
#endif // __SEQUENCE_XMI_H
