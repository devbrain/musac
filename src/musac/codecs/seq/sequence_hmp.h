#ifndef __SEQUENCE_HMP_H
#define __SEQUENCE_HMP_H

#include "sequence_mid.h"
namespace ymfmidi {
	class SequenceHMP : public SequenceMID
	{
		public:
		SequenceHMP();
		~SequenceHMP() override;

		void setTimePerBeat(uint32_t usec) override;

		static bool isValid(const uint8_t *data, size_t size);

		private:
		void read(const uint8_t *data, size_t size) override;
	};
}
#endif // __SEQUENCE_HMP_H
