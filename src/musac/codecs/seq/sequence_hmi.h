#ifndef __SEQUENCE_HMI_H
#define __SEQUENCE_HMI_H

#include "musac/codecs/seq/sequence_mid.h"

namespace ymfmidi {
	class SequenceHMI : public SequenceMID
	{
		public:
		SequenceHMI();
		~SequenceHMI() override;

		void setTimePerBeat(uint32_t usec) override;

		static bool isValid(const uint8_t *data, size_t size);

		private:
		void read(const uint8_t *data, size_t size) override;
	};
}
#endif // __SEQUENCE_HMP_H
