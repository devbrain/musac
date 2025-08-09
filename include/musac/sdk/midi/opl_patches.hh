#ifndef MUSAC_OPL_PATCHES_HH
#define MUSAC_OPL_PATCHES_HH

#include <musac/sdk/export_musac_sdk.h>
#include <musac/sdk/types.hh>
#include <string>
#include <unordered_map>

namespace musac {

class io_stream;

struct MUSAC_SDK_EXPORT patch_voice {
    uint8_t op_mode[2] = {0};  // regs 0x20+
    uint8_t op_ksr[2]   = {0}; // regs 0x40+ (upper bits)
    uint8_t op_level[2] = {0}; // regs 0x40+ (lower bits)
    uint8_t op_ad[2] = {0};    // regs 0x60+
    uint8_t op_sr[2] = {0};    // regs 0x80+
    uint8_t conn = 0;          // regs 0xC0+
    uint8_t op_wave[2] = {0};  // regs 0xE0+

    int8_t tune = 0; // MIDI note offset
    double finetune = 1.0; // frequency multiplier
};

struct MUSAC_SDK_EXPORT opl_patch {
    std::string name;
    bool four_op = false; // true 4op
    bool dual_two_op = false; // only valid if four_op = false
    uint8_t fixed_note = 0;
    int8_t velocity = 0; // MIDI velocity offset

    patch_voice voice[2];

    static const char* names[256];
};

using opl_patch_set = std::unordered_map<uint16_t, opl_patch>;

class MUSAC_SDK_EXPORT opl_patch_loader {
public:
    static bool load(opl_patch_set& patches, const char* path);
    static bool load(opl_patch_set& patches, FILE* file, int offset = 0, size_t size = 0);
    static bool load(opl_patch_set& patches, io_stream* file, int offset = 0, size_t size = 0);
    static bool load(opl_patch_set& patches, const uint8_t* data, size_t size);

private:
    static bool load_wopl(opl_patch_set& patches, const uint8_t* data, size_t size);
    static bool load_op2(opl_patch_set& patches, const uint8_t* data, size_t size);
    static bool load_ail(opl_patch_set& patches, const uint8_t* data, size_t size);
    static bool load_tmb(opl_patch_set& patches, const uint8_t* data, size_t size);
};

} // namespace musac

#endif // MUSAC_OPL_PATCHES_HH