//
// Created by igor on 3/23/25.
//

#ifndef  YMFM_CHIP_HH
#define  YMFM_CHIP_HH

#include <cstdint>
#include <string>
#include <vector>

#include "codecs/opl/ymfm/ymfm.h"


namespace musac {
    // we use an int64_t as emulated time, as a 32.32 fixed point value
    using emulated_time = int64_t;

    // enumeration of the different types of chips we support
    enum chip_type {
        CHIP_YM2149,
        CHIP_YM2151,
        CHIP_YM2203,
        CHIP_YM2413,
        CHIP_YM2608,
        CHIP_YM2610,
        CHIP_YM2612,
        CHIP_YM3526,
        CHIP_Y8950,
        CHIP_YM3812,
        CHIP_YMF262,
        CHIP_YMF278B,
        CHIP_TYPES
    };

    // abstract base class for a Yamaha chip; we keep a list of these for processing
    // as new commands come in
    class ymfm_chip_base {
        public:
            // construction
            ymfm_chip_base(uint32_t clock, chip_type type, char const* name);

            // destruction
            virtual ~ymfm_chip_base();

            // simple getters
            [[nodiscard]] chip_type type() const;
            [[nodiscard]] virtual uint32_t sample_rate() const = 0;

            // required methods for derived classes to implement
            virtual void write(uint32_t reg, uint8_t data) = 0;
            virtual void generate(emulated_time output_start, emulated_time output_step, int32_t* buffer) = 0;

            // write data to the ADPCM-A buffer
            void write_data(ymfm::access_class type, uint32_t base, uint32_t length, uint8_t const* src);

            // seek within the PCM stream
            void seek_pcm(uint32_t pos);
            uint8_t read_pcm();

        protected:
            // internal state
            chip_type m_type;
            std::string m_name;
            std::vector <uint8_t> m_data[ymfm::ACCESS_CLASSES];
            uint32_t m_pcm_offset;
    };

    // actual chip-specific implementation class; includes implementatino of the
    // ymfm_interface as needed for vgmplay purposes
    template<typename ChipType>
    class ymfm_chip : public ymfm_chip_base, public ymfm::ymfm_interface {
        public:
            // construction
            ymfm_chip(uint32_t clock_, chip_type type, char const* name)
                : ymfm_chip_base(clock_, type, name),
                  m_chip(*this),
                  m_clock(clock_),
                  m_clocks(0),
                  m_step(0x100000000ull / m_chip.sample_rate(clock_)),
                  m_pos(0) {
                m_chip.reset();
#define EXTRA_CLOCKS (0)
                for (int clock = 0; clock < EXTRA_CLOCKS; clock++)
                    m_chip.generate(&m_output);
            }

            [[nodiscard]] uint32_t sample_rate() const override {
                return m_chip.sample_rate(m_clock);
            }

            // handle a register write: just queue for now
            void write(uint32_t reg, uint8_t data) override {
                m_queue.emplace_back(reg, data);
            }

            // generate one output sample of output
            void generate(emulated_time output_start, emulated_time output_step, int32_t* buffer) override {
                uint32_t addr1 = 0xffff, addr2 = 0xffff;
                uint8_t data1 = 0, data2 = 0;

                // see if there is data to be written; if so, extract it and dequeue
                if (!m_queue.empty()) {
                    auto front = m_queue.front();
                    addr1 = 0 + 2 * ((front.first >> 8) & 3);
                    data1 = front.first & 0xff;
                    addr2 = addr1 + ((m_type == CHIP_YM2149) ? 2 : 1);
                    data2 = front.second;
                    m_queue.erase(m_queue.begin());
                }

                // write to the chip
                if (addr1 != 0xffff) {
                    m_chip.write(addr1, data1);
                    m_chip.write(addr2, data2);
                }

                // generate at the appropriate sample rate
                //		nuked::s_log_envelopes = (output_start >= (22ll << 32) && output_start < (24ll << 32));
                for (; m_pos <= output_start; m_pos += m_step) {
                    m_chip.generate(&m_output);
                }

                // add the final result to the buffer
                if (m_type == CHIP_YM2203) {
                    int32_t out0 = m_output.data[0];
                    int32_t out1 = m_output.data[1 % ChipType::OUTPUTS];
                    int32_t out2 = m_output.data[2 % ChipType::OUTPUTS];
                    int32_t out3 = m_output.data[3 % ChipType::OUTPUTS];
                    *buffer++ += out0 + out1 + out2 + out3;
                    *buffer++ += out0 + out1 + out2 + out3;
                } else if (m_type == CHIP_YM2608 || m_type == CHIP_YM2610) {
                    int32_t out0 = m_output.data[0];
                    int32_t out1 = m_output.data[1 % ChipType::OUTPUTS];
                    int32_t out2 = m_output.data[2 % ChipType::OUTPUTS];
                    *buffer++ += out0 + out2;
                    *buffer++ += out1 + out2;
                } else if (m_type == CHIP_YMF278B) {
                    *buffer++ += m_output.data[4 % ChipType::OUTPUTS];
                    *buffer++ += m_output.data[5 % ChipType::OUTPUTS];
                } else if (ChipType::OUTPUTS == 1) {
                    *buffer++ += m_output.data[0];
                    *buffer++ += m_output.data[0];
                } else {
                    *buffer++ += m_output.data[0];
                    *buffer++ += m_output.data[1 % ChipType::OUTPUTS];
                }
                m_clocks++;
            }

        protected:
            // handle a read from the buffer
            uint8_t ymfm_external_read(ymfm::access_class type, uint32_t offset) override {
                auto& data = m_data[type];
                return (offset < data.size()) ? data[offset] : 0;
            }

            // internal state
            ChipType m_chip;
            uint32_t m_clock;
            uint64_t m_clocks;
            typename ChipType::output_data m_output;
            emulated_time m_step;
            emulated_time m_pos;
            std::vector <std::pair <uint32_t, uint8_t>> m_queue;
    };
}

#endif
