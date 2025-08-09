//
// Created by igor on 3/23/25.
//

#include <failsafe/failsafe.hh>
#include <algorithm>
#include <cmath>

#include "musac/codecs/vgm/vgm_player.hh"

// Helper macro for unsupported chip warnings
#define WARN_UNSUPPORTED_CHIP(name) LOG_WARN("VGM", "Clock for " name " specified, but not supported")


// Define access class constants locally
namespace {
    enum access_class_type {
        ACCESS_IO = 0,
        ACCESS_ADPCM_A,
        ACCESS_ADPCM_B,
        ACCESS_PCM,
        ACCESS_CLASSES
    };
    
    // Clamp helper function
    inline int32_t clamp(int32_t value, int32_t minval, int32_t maxval) {
        if (value < minval) return minval;
        if (value > maxval) return maxval;
        return value;
    }
}
#include "musac/codecs/vgm/em_inflate.h"


namespace {
    //-------------------------------------------------
    //  parse_uint32 - parse a little-endian uint32_t
    //-------------------------------------------------

    uint32_t parse_uint32(const std::vector <uint8_t>& buffer, uint32_t& offset) {
        uint32_t result = buffer[offset++];
        result |= buffer[offset++] << 8;
        result |= buffer[offset++] << 16;
        result |= buffer[offset++] << 24;
        return result;
    }
}

namespace musac {
    vgm_player::vgm_player()
        : m_data_start(0),
          m_cmds_offset(0),
          m_remainig_delays(0),
          m_done(false),
          m_output_pos(0) {
    }

    bool vgm_player::load(io_stream* file) {
        auto file_size = file->get_size();
        if (file_size <= 0) {
            return false;
        }
        m_input.resize(file_size);
        if (file->read( m_input.data(), file_size) != static_cast<size_t>(file_size)) {
            return false;
        }
        if (m_input.size() >= 10 && m_input[0] == 0x1f && m_input[1] == 0x8b && m_input[2] == 0x08) {
            // copy the raw data to a new buffer
            std::vector <uint8_t> compressed = m_input;

            // determine uncompressed size and resize the buffer
            uint8_t* end = &compressed[compressed.size()];
            uint32_t uncompressed = end[-4] | (end[-3] << 8) | (end[-2] << 16) | (end[-1] << 24);
            if (static_cast<size_t>(file_size) < compressed.size() || file_size > 32 * 1024 * 1024) {
                return false;
            }
            m_input.resize(uncompressed);

            // decompress the data
            auto result = (int)em_inflate(&compressed[0], compressed.size(), &m_input[0], m_input.size());
            if (result == -1) {
                return false;
            }
        }
        // check the ID

        if (m_input.size() < 64 || m_input[0] != 'V' || m_input[1] != 'g' || m_input[2] != 'm' || m_input[3] != ' ') {
            return false;
        }
        uint32_t offset = 4;
        // +04: parse the size
        uint32_t size = parse_uint32(m_input, offset);
        if (offset - 4 + size > m_input.size()) {
            LOG_ERROR("VGM", "Total size for file is too small; file may be truncated");
            size = (uint32_t)(m_input.size() - 4);
        }
        m_input.resize(size + 4);

        // parse the header, creating any chips needed
        m_data_start = parse_header(m_input, offset);
        m_cmds_offset = m_data_start;

        // if no chips created, fail
        if (m_active_chips.empty()) {
            return false;
        }
        return true;
    }

    int vgm_player::render(float* buff, int len) {
        constexpr auto output_rate = 44100;
        constexpr emulated_time output_step = 0x100000000ull / output_rate;
        if (m_done && m_remainig_delays == 0) {
            return 0;
        }
        if (m_cmds_offset >= m_input.size() && m_remainig_delays == 0) {
            m_done = true;
            return 0;
        }
        int sample_pairs = len / 2;

        if (m_remainig_delays == 0) {
            int delay = -1;
            do {
                delay = apply_cmd(m_input, m_cmds_offset, m_done);
                if (m_done) {
                    return 0;
                }
            } while (delay < 0);

            int to_take = std::min(sample_pairs, delay);
            for (int i=0; i<to_take; i++) {
                int32_t outputs[2] = {0};
                for (auto& chip : m_active_chips) {
                    chip->generate(m_output_pos, output_step, outputs);
                }
                m_output_pos += output_step;
                buff[2*i] = (float)clamp(outputs[0], -32768, 32768) / 32768.0f;
                buff[2*i+1] = (float)clamp(outputs[1], -32768, 32768) / 32768.0f;
            }
            m_remainig_delays = delay - to_take;
            return 2*to_take;
        } else {
            int to_take = std::min(sample_pairs, m_remainig_delays);
            for (int i=0; i<to_take; i++) {
                int32_t outputs[2] = {0};
                for (auto& chip : m_active_chips) {
                    chip->generate(m_output_pos, output_step, outputs);
                }
                m_output_pos += output_step;
                buff[2*i] = (float)clamp(outputs[0], -32768, 32768) / 32768.0f;
                buff[2*i+1] = (float)clamp(outputs[1], -32768, 32768) / 32768.0f;
            }
            m_remainig_delays -= to_take;
            return 2*to_take;
        }
    }

    ymfm_chip_base* vgm_player::find_chip(chip_type type, uint8_t index) const {
        for (auto& chip : m_active_chips)
            if (chip->type() == type && index-- == 0)
                return chip.get();
        return nullptr;
    }

    void vgm_player::write_chip(chip_type type, uint8_t index, uint32_t reg, uint8_t data) const {
        auto* chip = find_chip(type, index);
        if (chip != nullptr) {
            chip->write(reg, data);
        }
    }

    uint32_t vgm_player::parse_header(const std::vector <uint8_t>& buffer, uint32_t& offset) {
        // +00: already checked the ID

        // +08: parse the version
        uint32_t version = parse_uint32(buffer, offset);
        if (version > 0x171)
            LOG_WARN("VGM", "Version > 1.71 detected, some things may not work");

        // +0C: SN76489 clock
        uint32_t clock = parse_uint32(buffer, offset);
        if (clock != 0)
            LOG_WARN("VGM", "Clock for SN76489 specified:", clock, "but not supported");

        // +10: YM2413 clock
        clock = parse_uint32(buffer, offset);
        if (clock != 0)
            add_chips(clock, CHIP_YM2413, "YM2413");

        // +14: GD3 offset
        uint32_t dummy = parse_uint32(buffer, offset);

        // +18: Total # samples
        dummy = parse_uint32(buffer, offset);

        // +1C: Loop offset
        dummy = parse_uint32(buffer, offset);

        // +20: Loop # samples
        dummy = parse_uint32(buffer, offset);

        // +24: Rate
        dummy = parse_uint32(buffer, offset);

        // +28: SN76489 feedback / SN76489 shift register width / SN76489 Flags
        dummy = parse_uint32(buffer, offset);

        // +2C: YM2612 clock
        clock = parse_uint32(buffer, offset);
        if (version >= 0x110 && clock != 0)
            add_chips(clock, CHIP_YM2612, "YM2612");

        // +30: YM2151 clock
        clock = parse_uint32(buffer, offset);
        if (version >= 0x110 && clock != 0)
            add_chips(clock, CHIP_YM2151, "YM2151");

        // +34: VGM data offset
        uint32_t data_start = parse_uint32(buffer, offset);
        data_start += offset - 4;
        if (version < 0x150)
            data_start = 0x40;

        // +38: Sega PCM clock
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            LOG_WARN("VGM", "Clock for Sega PCM specified, but not supported");

        // +3C: Sega PCM interface register
        dummy = parse_uint32(buffer, offset);

        // +40: RF5C68 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            LOG_WARN("VGM", "Clock for RF5C68 specified, but not supported");

        // +44: YM2203 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            add_chips(clock, CHIP_YM2203, "YM2203");

        // +48: YM2608 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            add_chips(clock, CHIP_YM2608, "YM2608");

        // +4C: YM2610/2610B clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0) {
            if (clock & 0x80000000)
                add_chips(clock, CHIP_YM2610, "YM2610B");
            else
                add_chips(clock, CHIP_YM2610, "YM2610");
        }

        // +50: YM3812 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            add_chips(clock, CHIP_YM3812, "YM3812");

        // +54: YM3526 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            add_chips(clock, CHIP_YM3526, "YM3526");

        // +58: Y8950 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            add_chips(clock, CHIP_Y8950, "Y8950");

        // +5C: YMF262 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            add_chips(clock, CHIP_YMF262, "YMF262");

        // +60: YMF278B clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            add_chips(clock, CHIP_YMF278B, "YMF278B");

        // +64: YMF271 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            WARN_UNSUPPORTED_CHIP("YMF271");

        // +68: YMF280B clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            WARN_UNSUPPORTED_CHIP("YMF280B");

        // +6C: RF5C164 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            WARN_UNSUPPORTED_CHIP("RF5C164");

        // +70: PWM clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0)
            WARN_UNSUPPORTED_CHIP("PWM");

        // +74: AY8910 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x151 && clock != 0) {
            LOG_WARN("VGM", "Clock for AY8910 specified, substituting YM2149");
            add_chips(clock, CHIP_YM2149, "YM2149");
        }

        // +78: AY8910 flags
        if (offset + 4 > data_start)
            return data_start;
        dummy = parse_uint32(buffer, offset);

        // +7C: volume / loop info
        if (offset + 4 > data_start)
            return data_start;
        dummy = parse_uint32(buffer, offset);
        if ((dummy & 0xff) != 0)
            LOG_DEBUG("VGM", "Volume modifier:", (dummy & 0xff), "(=", int(pow(2, double(dummy & 0xff) / 0x20)), ")");

        // +80: GameBoy DMG clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("GameBoy DMG");

        // +84: NES APU clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("NES APU");

        // +88: MultiPCM clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("MultiPCM");

        // +8C: uPD7759 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("uPD7759");

        // +90: OKIM6258 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("OKIM6258");

        // +94: OKIM6258 Flags / K054539 Flags / C140 Chip Type / reserved
        if (offset + 4 > data_start)
            return data_start;
        dummy = parse_uint32(buffer, offset);

        // +98: OKIM6295 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("OKIM6295");

        // +9C: K051649 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("K051649");

        // +A0: K054539 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("K054539");

        // +A4: HuC6280 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("HuC6280");

        // +A8: C140 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("C140");

        // +AC: K053260 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("K053260");

        // +B0: Pokey clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("Pokey");

        // +B4: QSound clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x161 && clock != 0)
            WARN_UNSUPPORTED_CHIP("QSound");

        // +B8: SCSP clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x171 && clock != 0)
            WARN_UNSUPPORTED_CHIP("SCSP");

        // +BC: extra header offset
        if (offset + 4 > data_start)
            return data_start;
        /*uint32_t extra_header =*/ parse_uint32(buffer, offset);

        // +C0: WonderSwan clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x171 && clock != 0)
            WARN_UNSUPPORTED_CHIP("WonderSwan");

        // +C4: VSU clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x171 && clock != 0)
            WARN_UNSUPPORTED_CHIP("VSU");

        // +C8: SAA1099 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x171 && clock != 0)
            WARN_UNSUPPORTED_CHIP("SAA1099");

        // +CC: ES5503 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x171 && clock != 0)
            WARN_UNSUPPORTED_CHIP("ES5503");

        // +D0: ES5505/ES5506 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x171 && clock != 0)
            WARN_UNSUPPORTED_CHIP("ES5505/ES5506");

        // +D4: ES5503 output channels / ES5505/ES5506 amount of output channels / C352 clock divider
        if (offset + 4 > data_start)
            return data_start;
        dummy = parse_uint32(buffer, offset);

        // +D8: X1-010 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x171 && clock != 0)
            WARN_UNSUPPORTED_CHIP("X1-010");

        // +DC: C352 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x171 && clock != 0)
            WARN_UNSUPPORTED_CHIP("C352");

        // +E0: GA20 clock
        if (offset + 4 > data_start)
            return data_start;
        clock = parse_uint32(buffer, offset);
        if (version >= 0x171 && clock != 0)
            WARN_UNSUPPORTED_CHIP("GA20");
        return data_start;
    }

    void vgm_player::generate_all(const std::vector <uint8_t>& buffer,
                                  uint32_t data_start, uint32_t output_rate,
                                  std::vector <int32_t>& wav_buffer) {
        // set the offset to the data start and go
        uint32_t offset = data_start;
        bool done = false;
        emulated_time output_step = 0x100000000ull / output_rate;
        emulated_time output_pos = 0;
        while (!done && offset < buffer.size()) {
            int delay = apply_cmd(buffer, offset, done);
            // handle delays
            while (delay-- != 0) {
               // bool more_remaining = false;
                int32_t outputs[2] = {0};
                for (auto& chip : m_active_chips)
                    chip->generate(output_pos, output_step, outputs);
                output_pos += output_step;
                wav_buffer.push_back(outputs[0]);
                wav_buffer.push_back(outputs[1]);
            }
        }
    }

    void vgm_player::add_rom_data(chip_type type, int access, const std::vector <uint8_t>& buffer,
                                  uint32_t& localoffset, uint32_t size) {
        /*uint32_t length = */parse_uint32(buffer, localoffset);
        uint32_t start = parse_uint32(buffer, localoffset);
        for (int index = 0; index < 2; index++) {
            auto* chip = find_chip(type, index);
            if (chip != nullptr)
                chip->write_data(access, start, size, &buffer[localoffset]);
        }
    }

    int vgm_player::apply_cmd(const std::vector <uint8_t>& buffer, uint32_t& offset, bool& done) {
        int delay = 0;
        uint8_t cmd = buffer[offset++];
        switch (cmd) {
            // YM2413, write value dd to register aa
            case 0x51:
            case 0xa1:
                write_chip(CHIP_YM2413, cmd >> 7, buffer[offset], buffer[offset + 1]);
                offset += 2;
                break;

            // YM2612 port 0, write value dd to register aa
            case 0x52:
            case 0xa2:
                write_chip(CHIP_YM2612, cmd >> 7, buffer[offset], buffer[offset + 1]);
                offset += 2;
                break;

            // YM2612 port 1, write value dd to register aa
            case 0x53:
            case 0xa3:
                write_chip(CHIP_YM2612, cmd >> 7, buffer[offset] | 0x100, buffer[offset + 1]);
                offset += 2;
                break;

            // YM2151, write value dd to register aa
            case 0x54:
            case 0xa4:
                write_chip(CHIP_YM2151, cmd >> 7, buffer[offset], buffer[offset + 1]);
                offset += 2;
                break;

            // YM2203, write value dd to register aa
            case 0x55:
            case 0xa5:
                write_chip(CHIP_YM2203, cmd >> 7, buffer[offset], buffer[offset + 1]);
                offset += 2;
                break;

            // YM2608 port 0, write value dd to register aa
            case 0x56:
            case 0xa6:
                write_chip(CHIP_YM2608, cmd >> 7, buffer[offset], buffer[offset + 1]);
                offset += 2;
                break;

            // YM2608 port 1, write value dd to register aa
            case 0x57:
            case 0xa7:
                write_chip(CHIP_YM2608, cmd >> 7, buffer[offset] | 0x100, buffer[offset + 1]);
                offset += 2;
                break;

            // YM2610 port 0, write value dd to register aa
            case 0x58:
            case 0xa8:
                write_chip(CHIP_YM2610, cmd >> 7, buffer[offset], buffer[offset + 1]);
                offset += 2;
                break;

            // YM2610 port 1, write value dd to register aa
            case 0x59:
            case 0xa9:
                write_chip(CHIP_YM2610, cmd >> 7, buffer[offset] | 0x100, buffer[offset + 1]);
                offset += 2;
                break;

            // YM3812, write value dd to register aa
            case 0x5a:
            case 0xaa:
                write_chip(CHIP_YM3812, cmd >> 7, buffer[offset], buffer[offset + 1]);
                offset += 2;
                break;

            // YM3526, write value dd to register aa
            case 0x5b:
            case 0xab:
                write_chip(CHIP_YM3526, cmd >> 7, buffer[offset], buffer[offset + 1]);
                offset += 2;
                break;

            // Y8950, write value dd to register aa
            case 0x5c:
            case 0xac:
                write_chip(CHIP_Y8950, cmd >> 7, buffer[offset], buffer[offset + 1]);
                offset += 2;
                break;

            // YMF262 port 0, write value dd to register aa
            case 0x5e:
            case 0xae:
                write_chip(CHIP_YMF262, cmd >> 7, buffer[offset], buffer[offset + 1]);
                offset += 2;
                break;

            // YMF262 port 1, write value dd to register aa
            case 0x5f:
            case 0xaf:
                write_chip(CHIP_YMF262, cmd >> 7, buffer[offset] | 0x100, buffer[offset + 1]);
                offset += 2;
                break;

            // Wait n samples, n can range from 0 to 65535 (approx 1.49 seconds)
            case 0x61:
                delay = buffer[offset] | (buffer[offset + 1] << 8);
                offset += 2;
                break;

            // wait 735 samples (60th of a second)
            case 0x62:
                delay = 735;
                break;

            // wait 882 samples (50th of a second)
            case 0x63:
                delay = 882;
                break;

            // end of sound data
            case 0x66:
                done = true;
                break;

            // data block
            case 0x67: {
                uint8_t dummy = buffer[offset++];
                if (dummy != 0x66)
                    break;
                uint8_t type = buffer[offset++];
                uint32_t size = parse_uint32(buffer, offset);
                uint32_t localoffset = offset;

                switch (type) {
                    case 0x01: // RF5C68 PCM data for use with associated commands
                    case 0x02: // RF5C164 PCM data for use with associated commands
                    case 0x03: // PWM PCM data for use with associated commands
                    case 0x04: // OKIM6258 ADPCM data for use with associated commands
                    case 0x05: // HuC6280 PCM data for use with associated commands
                    case 0x06: // SCSP PCM data for use with associated commands
                    case 0x07: // NES APU DPCM data for use with associated commands
                        break;

                    case 0x00: // YM2612 PCM data for use with associated commands
                    {
                        auto* chip = find_chip(CHIP_YM2612, 0);
                        if (chip != nullptr)
                            chip->write_data(ACCESS_PCM, 0, size - 8, &buffer[localoffset]);
                        break;
                    }

                    case 0x82: // YM2610 ADPCM ROM data
                        add_rom_data(CHIP_YM2610, ACCESS_ADPCM_A, buffer, localoffset, size - 8);
                        break;

                    case 0x81: // YM2608 DELTA-T ROM data
                        add_rom_data(CHIP_YM2608, ACCESS_ADPCM_B, buffer, localoffset, size - 8);
                        break;

                    case 0x83: // YM2610 DELTA-T ROM data
                        add_rom_data(CHIP_YM2610, ACCESS_ADPCM_B, buffer, localoffset, size - 8);
                        break;

                    case 0x84: // YMF278B ROM data
                    case 0x87: // YMF278B RAM data
                        add_rom_data(CHIP_YMF278B, ACCESS_PCM, buffer, localoffset, size - 8);
                        break;

                    case 0x88: // Y8950 DELTA-T ROM data
                        add_rom_data(CHIP_Y8950, ACCESS_ADPCM_B, buffer, localoffset, size - 8);
                        break;

                    case 0x80: // Sega PCM ROM data
                    case 0x85: // YMF271 ROM data
                    case 0x86: // YMZ280B ROM data
                    case 0x89: // MultiPCM ROM data
                    case 0x8A: // uPD7759 ROM data
                    case 0x8B: // OKIM6295 ROM data
                    case 0x8C: // K054539 ROM data
                    case 0x8D: // C140 ROM data
                    case 0x8E: // K053260 ROM data
                    case 0x8F: // Q-Sound ROM data
                    case 0x90: // ES5505/ES5506 ROM data
                    case 0x91: // X1-010 ROM data
                    case 0x92: // C352 ROM data
                    case 0x93: // GA20 ROM data
                        break;

                    case 0xC0: // RF5C68 RAM write
                    case 0xC1: // RF5C164 RAM write
                    case 0xC2: // NES APU RAM write
                    case 0xE0: // SCSP RAM write
                    case 0xE1: // ES5503 RAM write
                        break;

                    default:
                        // Ignore unsupported data blocks
                        break;
                }
                offset += size;
                break;
            }

            // PCM RAM write
            case 0x68:
                // Ignore PCM RAM writes for now
                break;

            // AY8910, write value dd to register aa
            case 0xa0:
                write_chip(CHIP_YM2149, buffer[offset] >> 7, buffer[offset] & 0x7f, buffer[offset + 1]);
                offset += 2;
                break;

            // pp aa dd: YMF278B, port pp, write value dd to register aa
            case 0xd0:
                write_chip(CHIP_YMF278B, buffer[offset] >> 7, ((buffer[offset] & 0x7f) << 8) | buffer[offset + 1],
                           buffer[offset + 2]);
                offset += 3;
                break;

            case 0x70:
            case 0x71:
            case 0x72:
            case 0x73:
            case 0x74:
            case 0x75:
            case 0x76:
            case 0x77:
            case 0x78:
            case 0x79:
            case 0x7a:
            case 0x7b:
            case 0x7c:
            case 0x7d:
            case 0x7e:
            case 0x7f:
                delay = (cmd & 15) + 1;
                break;

            case 0x80:
            case 0x81:
            case 0x82:
            case 0x83:
            case 0x84:
            case 0x85:
            case 0x86:
            case 0x87:
            case 0x88:
            case 0x89:
            case 0x8a:
            case 0x8b:
            case 0x8c:
            case 0x8d:
            case 0x8e:
            case 0x8f: {
                auto* chip = find_chip(CHIP_YM2612, 0);
                if (chip != nullptr)
                    chip->write(0x2a, chip->read_pcm());
                delay = cmd & 15;
                break;
            }

            // ignored, consume one byte
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x36:
            case 0x37:
            case 0x38:
            case 0x39:
            case 0x3a:
            case 0x3b:
            case 0x3c:
            case 0x3d:
            case 0x3e:
            case 0x3f:
            case 0x4f: // dd: Game Gear PSG stereo, write dd to port 0x06
            case 0x50: // dd: PSG (SN76489/SN76496) write value dd
                offset++;
                break;

            // ignored, consume two bytes
            case 0x40:
            case 0x41:
            case 0x42:
            case 0x43:
            case 0x44:
            case 0x45:
            case 0x46:
            case 0x47:
            case 0x48:
            case 0x49:
            case 0x4a:
            case 0x4b:
            case 0x4c:
            case 0x4d:
            case 0x4e:
            case 0x5d: // aa dd: YMZ280B, write value dd to register aa
            case 0xb0: // aa dd: RF5C68, write value dd to register aa
            case 0xb1: // aa dd: RF5C164, write value dd to register aa
            case 0xb2: // aa dd: PWM, write value ddd to register a (d is MSB, dd is LSB)
            case 0xb3: // aa dd: GameBoy DMG, write value dd to register aa
            case 0xb4: // aa dd: NES APU, write value dd to register aa
            case 0xb5: // aa dd: MultiPCM, write value dd to register aa
            case 0xb6: // aa dd: uPD7759, write value dd to register aa
            case 0xb7: // aa dd: OKIM6258, write value dd to register aa
            case 0xb8: // aa dd: OKIM6295, write value dd to register aa
            case 0xb9: // aa dd: HuC6280, write value dd to register aa
            case 0xba: // aa dd: K053260, write value dd to register aa
            case 0xbb: // aa dd: Pokey, write value dd to register aa
            case 0xbc: // aa dd: WonderSwan, write value dd to register aa
            case 0xbd: // aa dd: SAA1099, write value dd to register aa
            case 0xbe: // aa dd: ES5506, write value dd to register aa
            case 0xbf: // aa dd: GA20, write value dd to register aa
                offset += 2;
                break;

            // ignored, consume three bytes
            case 0xc9:
            case 0xca:
            case 0xcb:
            case 0xcc:
            case 0xcd:
            case 0xce:
            case 0xcf:
            case 0xd7:
            case 0xd8:
            case 0xd9:
            case 0xda:
            case 0xdb:
            case 0xdc:
            case 0xdd:
            case 0xde:
            case 0xdf:
            case 0xc0: // bbaa dd: Sega PCM, write value dd to memory offset aabb
            case 0xc1: // bbaa dd: RF5C68, write value dd to memory offset aabb
            case 0xc2: // bbaa dd: RF5C164, write value dd to memory offset aabb
            case 0xc3: // cc bbaa: MultiPCM, write set bank offset aabb to channel cc
            case 0xc4: // mmll rr: QSound, write value mmll to register rr (mm - data MSB, ll - data LSB)
            case 0xc5: // mmll dd: SCSP, write value dd to memory offset mmll (mm - offset MSB, ll - offset LSB)
            case 0xc6:
            // mmll dd: WonderSwan, write value dd to memory offset mmll (mm - offset MSB, ll - offset LSB)
            case 0xc7: // mmll dd: VSU, write value dd to memory offset mmll (mm - offset MSB, ll - offset LSB)
            case 0xc8: // mmll dd: X1-010, write value dd to memory offset mmll (mm - offset MSB, ll - offset LSB)
            case 0xd1: // pp aa dd: YMF271, port pp, write value dd to register aa
            case 0xd2: // pp aa dd: SCC1, port pp, write value dd to register aa
            case 0xd3: // pp aa dd: K054539, write value dd to register ppaa
            case 0xd4: // pp aa dd: C140, write value dd to register ppaa
            case 0xd5: // pp aa dd: ES5503, write value dd to register ppaa
            case 0xd6: // pp aa dd: ES5506, write value aadd to register pp
                offset += 3;
                break;

            // ignored, consume four bytes
            case 0xe0:
                // dddddddd: Seek to offset dddddddd (Intel byte order) in PCM data bank of data block type 0 (YM2612).
            {
                auto* chip = find_chip(CHIP_YM2612, 0);
                uint32_t pos = parse_uint32(buffer, offset);
                if (chip != nullptr)
                    chip->seek_pcm(pos);
                offset += 4;
                break;
            }
            case 0xe1: // mmll aadd: C352, write value aadd to register mmll
            case 0xe2:
            case 0xe3:
            case 0xe4:
            case 0xe5:
            case 0xe6:
            case 0xe7:
            case 0xe8:
            case 0xe9:
            case 0xea:
            case 0xeb:
            case 0xec:
            case 0xed:
            case 0xee:
            case 0xef:
            case 0xf0:
            case 0xf1:
            case 0xf2:
            case 0xf3:
            case 0xf4:
            case 0xf5:
            case 0xf6:
            case 0xf7:
            case 0xf8:
            case 0xf9:
            case 0xfa:
            case 0xfb:
            case 0xfc:
            case 0xfd:
            case 0xfe:
            case 0xff:
                offset += 4;
                break;
        }
        return delay;
    }
}
