#ifndef MUSAC_CHIP_EMULATOR_HH
#define MUSAC_CHIP_EMULATOR_HH

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <musac/sdk/export_musac_sdk.h>

namespace musac {

// Enumeration of supported chip types
enum class MUSAC_SDK_EXPORT chip_type_id {
    YM2149,   // AY-3-8910 compatible
    YM2151,   // OPM
    YM2203,   // OPN
    YM2413,   // OPLL
    YM2608,   // OPNA
    YM2610,   // OPNB
    YM2612,   // OPN2
    YM3526,   // OPL
    Y8950,    // MSX-AUDIO
    YM3812,   // OPL2
    YMF262,   // OPL3
    YMF278B,  // OPL4
    CHIP_TYPES_COUNT
};

// Abstract base class for chip emulation
class MUSAC_SDK_EXPORT chip_emulator {
public:
    virtual ~chip_emulator() = default;
    
    // Get the chip type
    [[nodiscard]] virtual chip_type_id type() const = 0;
    
    // Get the chip's native sample rate
    [[nodiscard]] virtual uint32_t sample_rate() const = 0;
    
    // Get number of outputs (1=mono, 2=stereo, etc.)
    [[nodiscard]] virtual uint32_t num_outputs() const = 0;
    
    // Reset the chip to initial state
    virtual void reset() = 0;
    
    // Write a value to a register
    virtual void write(uint32_t offset, uint8_t data) = 0;
    
    // Generate samples (interleaved if stereo)
    // Returns actual number of samples generated
    virtual uint32_t generate(int32_t* buffer, uint32_t num_samples) = 0;
    
    // Get chip name for debugging
    [[nodiscard]] virtual std::string name() const = 0;
    
    // Set silent mode (for fast-forward operations)
    virtual void set_silent_mode(bool enable) = 0;
    
    // Get silent mode status
    [[nodiscard]] virtual bool get_silent_mode() const = 0;
};

// Factory function to create chip emulators
MUSAC_SDK_EXPORT std::unique_ptr<chip_emulator> create_chip_emulator(
    chip_type_id type, 
    uint32_t clock_rate);

// Helper to get chip type name
MUSAC_SDK_EXPORT const char* get_chip_type_name(chip_type_id type);

// Helper to check if a chip type supports stereo output
MUSAC_SDK_EXPORT bool chip_type_is_stereo(chip_type_id type);

} // namespace musac

#endif // MUSAC_CHIP_EMULATOR_HH