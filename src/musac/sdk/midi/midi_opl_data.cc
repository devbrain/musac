#include <musac/sdk/export_musac_sdk.h>

namespace musac {

// Include the MIDI OPL patch data
#include "midi_opl_data.h"

// Export the data for use by the library
extern "C" {
    MUSAC_SDK_EXPORT const unsigned char* get_genmidi_wopl_data() {
        return GENMIDI_wopl;
    }
    
    MUSAC_SDK_EXPORT unsigned int get_genmidi_wopl_size() {
        return sizeof(GENMIDI_wopl);
    }
}

} // namespace musac