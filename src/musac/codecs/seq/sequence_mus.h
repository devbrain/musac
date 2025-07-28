#ifndef __SEQUENCE_MUS_H
#define __SEQUENCE_MUS_H

#include "sequence.h"
namespace ymfmidi {
	class SequenceMUS : public Sequence
	{
		public:
		SequenceMUS();

		void reset() override;
		uint32_t update(OPLPlayer& player) override;

		static bool isValid(const uint8_t *data, size_t size);

		private:
		void read(const uint8_t *data, size_t size) override;
		void setDefaults();

		uint8_t m_data[1 << 16]{};
		uint16_t m_pos{};
		uint8_t m_lastVol[16]{};
	};
}
#endif // __SEQUENCE_MUS_H
