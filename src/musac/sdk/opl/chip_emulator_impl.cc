#include <musac/sdk/opl/chip_emulator.hh>
#include <stdexcept>
#include <cstring>

// Include internal YMFM headers
#include "internal/ymfm/ymfm.h"
#include "internal/ymfm/ymfm_opl.h"
#include "internal/ymfm/ymfm_opm.h"
#include "internal/ymfm/ymfm_opn.h"
#include "internal/ymfm/ymfm_misc.h"

namespace musac {

// Forward declaration
template<typename ChipType>
class ymfm_chip_emulator;

// Helper class to handle YMFM interface
class ymfm_interface : public ymfm::ymfm_interface {
public:
    virtual ~ymfm_interface() = default;
    
    // YMFM interface implementation
    virtual void ymfm_sync_mode_write(uint8_t data) override { (void)data; }
    virtual void ymfm_sync_check_interrupts() override { }
    virtual void ymfm_set_timer(uint32_t tnum, int32_t duration_in_clocks) override { (void)tnum; (void)duration_in_clocks; }
    virtual void ymfm_update_irq(bool asserted) override { (void)asserted; }
    virtual uint8_t ymfm_external_read(ymfm::access_class type, uint32_t address) override { 
        (void)type; (void)address; 
        return 0; 
    }
    virtual void ymfm_external_write(ymfm::access_class type, uint32_t address, uint8_t data) override { 
        (void)type; (void)address; (void)data; 
    }
};

// Template implementation for YMFM chips
template<typename ChipType>
class ymfm_chip_emulator : public chip_emulator {
private:
    chip_type_id m_type;
    uint32_t m_clock_rate;
    std::string m_name;
    ymfm_interface m_interface;
    ChipType m_chip;
    bool m_silent_mode;
    
public:
    ymfm_chip_emulator(chip_type_id type, uint32_t clock_rate, const char* name)
        : m_type(type)
        , m_clock_rate(clock_rate)
        , m_name(name)
        , m_chip(m_interface)
        , m_silent_mode(false) {
        m_chip.reset();
    }
    
    chip_type_id type() const override {
        return m_type;
    }
    
    uint32_t sample_rate() const override {
        return m_chip.sample_rate(m_clock_rate);
    }
    
    uint32_t num_outputs() const override {
        return ChipType::OUTPUTS;
    }
    
    void reset() override {
        m_chip.reset();
    }
    
    void write(uint32_t offset, uint8_t data) override {
        m_chip.write(offset, data);
    }
    
    uint32_t generate(int32_t* buffer, uint32_t num_samples) override {
        if (m_silent_mode) {
            // In silent mode, just fill with zeros
            std::memset(buffer, 0, num_samples * sizeof(int32_t));
            return num_samples;
        }
        
        typename ChipType::output_data output;
        uint32_t generated = 0;
        
        for (uint32_t i = 0; i < num_samples; i += ChipType::OUTPUTS) {
            m_chip.generate(&output);
            
            // Copy output data to buffer
            for (uint32_t j = 0; j < ChipType::OUTPUTS && (i + j) < num_samples; ++j) {
                buffer[i + j] = output.data[j];
                generated++;
            }
        }
        
        return generated;
    }
    
    std::string name() const override {
        return m_name;
    }
    
    void set_silent_mode(bool enable) override {
        m_silent_mode = enable;
    }
    
    bool get_silent_mode() const override {
        return m_silent_mode;
    }
};

// Factory function implementation
std::unique_ptr<chip_emulator> create_chip_emulator(chip_type_id type, uint32_t clock_rate) {
    switch (type) {
        case chip_type_id::YM2149:
            return std::make_unique<ymfm_chip_emulator<ymfm::ym2149>>(
                type, clock_rate, "YM2149");
                
        case chip_type_id::YM2151:
            return std::make_unique<ymfm_chip_emulator<ymfm::ym2151>>(
                type, clock_rate, "YM2151");
                
        case chip_type_id::YM2203:
            return std::make_unique<ymfm_chip_emulator<ymfm::ym2203>>(
                type, clock_rate, "YM2203");
                
        case chip_type_id::YM2413:
            return std::make_unique<ymfm_chip_emulator<ymfm::ym2413>>(
                type, clock_rate, "YM2413");
                
        case chip_type_id::YM2608:
            return std::make_unique<ymfm_chip_emulator<ymfm::ym2608>>(
                type, clock_rate, "YM2608");
                
        case chip_type_id::YM2610:
            return std::make_unique<ymfm_chip_emulator<ymfm::ym2610>>(
                type, clock_rate, "YM2610");
                
        case chip_type_id::YM2612:
            return std::make_unique<ymfm_chip_emulator<ymfm::ym2612>>(
                type, clock_rate, "YM2612");
                
        case chip_type_id::YM3526:
            return std::make_unique<ymfm_chip_emulator<ymfm::ym3526>>(
                type, clock_rate, "YM3526");
                
        case chip_type_id::Y8950:
            return std::make_unique<ymfm_chip_emulator<ymfm::y8950>>(
                type, clock_rate, "Y8950");
                
        case chip_type_id::YM3812:
            return std::make_unique<ymfm_chip_emulator<ymfm::ym3812>>(
                type, clock_rate, "YM3812");
                
        case chip_type_id::YMF262:
            return std::make_unique<ymfm_chip_emulator<ymfm::ymf262>>(
                type, clock_rate, "YMF262");
                
        case chip_type_id::YMF278B:
            return std::make_unique<ymfm_chip_emulator<ymfm::ymf278b>>(
                type, clock_rate, "YMF278B");
                
        default:
            throw std::invalid_argument("Unsupported chip type");
    }
}

// Helper functions
const char* get_chip_type_name(chip_type_id type) {
    switch (type) {
        case chip_type_id::YM2149:  return "YM2149";
        case chip_type_id::YM2151:  return "YM2151";
        case chip_type_id::YM2203:  return "YM2203";
        case chip_type_id::YM2413:  return "YM2413";
        case chip_type_id::YM2608:  return "YM2608";
        case chip_type_id::YM2610:  return "YM2610";
        case chip_type_id::YM2612:  return "YM2612";
        case chip_type_id::YM3526:  return "YM3526";
        case chip_type_id::Y8950:   return "Y8950";
        case chip_type_id::YM3812:  return "YM3812";
        case chip_type_id::YMF262:  return "YMF262";
        case chip_type_id::YMF278B: return "YMF278B";
        default:                    return "Unknown";
    }
}

bool chip_type_is_stereo(chip_type_id type) {
    switch (type) {
        case chip_type_id::YM2151:
        case chip_type_id::YM2608:
        case chip_type_id::YM2610:
        case chip_type_id::YM2612:
        case chip_type_id::YMF262:
        case chip_type_id::YMF278B:
            return true;
        default:
            return false;
    }
}

} // namespace musac