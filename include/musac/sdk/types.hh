/**
 * @file types.hh
 * @brief Platform-independent type definitions
 * @ingroup sdk_types
 */

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

/**
 * @defgroup sdk_types Type Definitions
 * @ingroup sdk
 * @brief Core type definitions for audio processing
 * 
 * The musac library uses specific type aliases for audio-related values
 * to provide clarity, type safety, and platform independence.
 * 
 * ## Why Custom Types?
 * 
 * 1. **Semantic Clarity**: `sample_rate_t` is clearer than `uint32_t`
 * 2. **Type Safety**: Prevents mixing incompatible values
 * 3. **Future-proofing**: Can change underlying type without breaking API
 * 4. **Documentation**: Types document their purpose
 * 5. **C Compatibility**: Header works in both C and C++
 * 
 * ## Usage Example
 * 
 * @code
 * sample_rate_t rate = 44100;  // CD quality
 * channels_t channels = 2;      // Stereo
 * 
 * // Type safety prevents mistakes
 * // channels_t ch = rate;  // Compilation error!
 * 
 * // Use in function signatures
 * void setup_audio(sample_rate_t rate, channels_t channels);
 * @endcode
 * 
 * @{
 */

/**
 * @typedef sample_rate_t
 * @brief Type for audio sample rates
 * 
 * Represents the number of audio samples per second (Hz).
 * Common values include:
 * - 8000 Hz: Telephone quality
 * - 22050 Hz: AM radio quality  
 * - 44100 Hz: CD quality
 * - 48000 Hz: Professional audio
 * - 96000 Hz: High-resolution audio
 * - 192000 Hz: Studio mastering
 * 
 * Range: 1 to 4,294,967,295 Hz (uint32_t max)
 * 
 * @note Most hardware supports 44100 and 48000 Hz
 */
using sample_rate_t = uint32_t;

/**
 * @typedef channels_t
 * @brief Type for audio channel count
 * 
 * Represents the number of audio channels in a stream.
 * Common configurations:
 * - 1: Mono (single channel)
 * - 2: Stereo (left/right)
 * - 4: Quadraphonic (front L/R, rear L/R)
 * - 6: 5.1 surround (FL/FR/FC/LFE/RL/RR)
 * - 8: 7.1 surround (FL/FR/FC/LFE/RL/RR/SL/SR)
 * 
 * Range: 1 to 255 channels (uint8_t max)
 * 
 * @note Most content is stereo (2 channels)
 */
using channels_t = uint8_t;

/** @} */ // end of sdk_types group

} // namespace musac
#endif

#endif // MUSAC_SDK_TYPES_H