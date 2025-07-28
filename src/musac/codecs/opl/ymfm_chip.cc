//
// Created by igor on 3/23/25.
//

#include "ymfm_chip.hh"

namespace musac {
    ymfm_chip_base::ymfm_chip_base(uint32_t clock, chip_type type, char const* name)
        : m_type(type),
          m_name(name), m_pcm_offset(0) {
    }

    ymfm_chip_base::~ymfm_chip_base() = default;

    chip_type ymfm_chip_base::type() const { return m_type; }

    void ymfm_chip_base::write_data(ymfm::access_class type, uint32_t base, uint32_t length, uint8_t const* src) {
        uint32_t end = base + length;
        if (end > m_data[type].size())
            m_data[type].resize(end);
        memcpy(&m_data[type][base], src, length);
    }

    void ymfm_chip_base::seek_pcm(uint32_t pos) { m_pcm_offset = pos; }

    uint8_t ymfm_chip_base::read_pcm() {
        auto& pcm = m_data[ymfm::ACCESS_PCM];
        return (m_pcm_offset < pcm.size()) ? pcm[m_pcm_offset++] : 0;
    }
}
