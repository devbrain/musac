//
// Created by igor on 3/23/25.
//

#ifndef  VGM_PLAYER_HH
#define  VGM_PLAYER_HH


#include <vector>
#include <memory>
#include <string>
#include <musac/sdk/io_stream.hh>
#include <musac/sdk/opl/ymfm_chip.hh>

#if defined(_MSC_VER)
#pragma warning (disable : 4996)
#endif


class vgm_chip_base;

namespace musac {
    class vgm_player {
        public:
            vgm_player();
            bool load(io_stream* file);

            int render(float* buff, int len);
            bool done() const {
                return m_done;
            }
        private:
            template<typename ChipType>
            void add_chips(uint32_t clock, chip_type type, char const* chipname) {
                uint32_t clockval = clock & 0x3fffffff;
                int numchips = (clock & 0x40000000) ? 2 : 1;
                for (int index = 0; index < numchips; index++) {
                    std::string name = (numchips == 2) ? 
                        std::string(chipname) + " #" + std::to_string(index) : chipname;
                    m_active_chips.push_back(
                        std::make_unique <ymfm_chip <ChipType>>(clockval, type, name.c_str()));
                }
            }

            [[nodiscard]] ymfm_chip_base* find_chip(chip_type type, uint8_t index) const;

            //-------------------------------------------------
            //  write_chip - handle a write to the given chip
            //  and index
            //-------------------------------------------------

            void write_chip(chip_type type, uint8_t index, uint32_t reg, uint8_t data) const;

            uint32_t parse_header(const std::vector <uint8_t>& buffer, uint32_t& offset);
            void generate_all(const std::vector <uint8_t>& buffer,
                              uint32_t data_start,
                              uint32_t output_rate,
                              std::vector <int32_t>& wav_buffer);

            void add_rom_data(chip_type type, ymfm::access_class access,
                              const std::vector <uint8_t>& buffer, uint32_t& localoffset, uint32_t size);

            int apply_cmd(const std::vector <uint8_t>& buffer, uint32_t& offset, bool& done);

        private:
            std::vector <std::unique_ptr <ymfm_chip_base>> m_active_chips;
            std::vector <uint8_t> m_input;
            uint32_t m_data_start;
            uint32_t m_cmds_offset;
            int m_remainig_delays;
            bool m_done;
            emulated_time m_output_pos;
    };
}

#endif
