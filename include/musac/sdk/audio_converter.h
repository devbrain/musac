#ifndef MUSAC_SDK_AUDIO_CONVERTER_H
#define MUSAC_SDK_AUDIO_CONVERTER_H

#include <musac/sdk/types.h>
#include <musac/sdk/audio_format.h>
#include <musac/sdk/export_musac_sdk.h>

namespace musac {

/**
 * Convert audio samples from one format to another.
 * 
 * This function supports format conversion, channel mixing, and sample rate conversion
 * using high-quality cubic interpolation (Catmull-Rom spline).
 * 
 * @param src_spec Source audio specification
 * @param src_data Source audio data
 * @param src_len Length of source data in bytes
 * @param dst_spec Destination audio specification
 * @param dst_data Pointer to receive allocated destination buffer (caller must delete[])
 * @param dst_len Pointer to receive destination data length in bytes
 * @return 1 on success, 0 on failure
 */
MUSAC_SDK_EXPORT int convert_audio_samples(const audio_spec* src_spec, const uint8* src_data, int src_len,
                                          const audio_spec* dst_spec, uint8** dst_data, int* dst_len);

} // namespace musac

#endif // MUSAC_SDK_AUDIO_CONVERTER_H