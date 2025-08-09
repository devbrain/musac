//
// Created by igor on 3/23/25.
//

#include <musac/sdk/opl/adlib_emu.h>
#include <musac/sdk/opl/ymfm_chip.hh>
#include "internal/ymfm/ymfm_opl.h"

namespace musac::impl {
    struct adlib {
        adlib()
            : m_chip(14318181, CHIP_YM3812, "YM3812"),
              m_step(0) {
        }
        ~adlib() = default;

        ymfm_chip<ymfm::ym3812> m_chip;
        static constexpr emulated_time output_step = 0x100000000ull / 44100;
        emulated_time m_step;
    };
}

void* adlib_init(unsigned samplerate) {
    auto* sb = new musac::impl::adlib;
    return sb;
}

void adlib_destroy(void* sb) {
    if (sb) {
        auto* self = (musac::impl::adlib*)sb;
        delete self;
    }
}

void adlib_write_data(void* sb, uint16_t reg, uint8_t val) {
    auto* self = (musac::impl::adlib*)sb;
    self->m_chip.write(reg, val);
}

void adlib_get_sample_stereo(void* sb, float* first, float* second) {
    auto* self = (musac::impl::adlib*)sb;
    int32_t outputs[2] = {0};
    self->m_chip.generate(self->m_step, musac::impl::adlib::output_step, outputs);
    
    self->m_step += musac::impl::adlib::output_step;
    
    // Clamp and normalize
    auto clamp = [](int32_t val) -> int32_t {
        return (val < -32768) ? -32768 : (val > 32767) ? 32767 : val;
    };
    
    *first = static_cast<float>(clamp(outputs[0])) / 32768.0f;
    *second = static_cast<float>(clamp(outputs[1])) / 32768.0f;
}

float adlib_get_sample(void* sb) {
    float first, second;
    adlib_get_sample_stereo(sb, &first, &second);
    return 0.5f*first + 0.5f*second;
}