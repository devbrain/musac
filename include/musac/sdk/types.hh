#ifndef MUSAC_SDK_TYPES_H
#define MUSAC_SDK_TYPES_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
namespace musac {

// Audio format types
using sample_rate_t = uint32_t;  // Sample rates (e.g., 44100, 48000, 96000 Hz)
using channels_t = uint8_t;      // Number of audio channels (e.g., 1=mono, 2=stereo, 6=5.1)

} // namespace musac
#endif

#endif // MUSAC_SDK_TYPES_H