/**
 * @file audio_format.hh
 * @brief Audio sample format definitions and utilities
 * @ingroup sdk_audio_format
 */

#ifndef MUSAC_SDK_AUDIO_FORMAT_H
#define MUSAC_SDK_AUDIO_FORMAT_H

#include <musac/sdk/types.hh>
#include <musac/sdk/musac_sdk_config.h>
#include <musac/sdk/export_musac_sdk.h>
#include <iosfwd>

namespace musac {

/**
 * @defgroup sdk_audio_format Audio Formats
 * @ingroup sdk
 * @brief Audio sample format definitions and conversion utilities
 * @{
 */

/**
 * @enum audio_format
 * @brief Audio sample format enumeration
 * 
 * Defines supported audio sample formats with encoded properties.
 * The format value encodes multiple properties in a single uint16_t:
 * 
 * - Bits 0-7: Bit size (8, 16, 32)
 * - Bit 8: Float flag (0=integer, 1=float)
 * - Bit 12: Endian flag (0=little, 1=big)
 * - Bit 15: Signed flag (0=unsigned, 1=signed)
 * 
 * ## Usage Example
 * 
 * @code
 * audio_format fmt = audio_format::s16le;  // CD quality format
 * 
 * // Query format properties
 * if (audio_format_is_float(fmt)) {
 *     // Handle floating-point samples
 * }
 * 
 * size_t bytes = audio_format_byte_size(fmt);  // Returns 2
 * bool big = audio_format_is_big_endian(fmt);  // Returns false
 * @endcode
 */
enum class audio_format : uint16_t {
    unknown = 0,          ///< Unknown or uninitialized format
    u8 = 0x0008,         ///< Unsigned 8-bit (0-255 range)
    s8 = 0x8008,         ///< Signed 8-bit (-128 to 127 range)
    s16le = 0x8010,      ///< Signed 16-bit little-endian (CD/WAV standard)
    s16be = 0x9010,      ///< Signed 16-bit big-endian (AIFF standard)
    s32le = 0x8020,      ///< Signed 32-bit little-endian (high quality)
    s32be = 0x9020,      ///< Signed 32-bit big-endian
    f32le = 0x8120,      ///< Float 32-bit little-endian (professional)
    f32be = 0x9120       ///< Float 32-bit big-endian
};

/**
 * @brief Get bit size of audio format
 * @param fmt Audio format
 * @return Number of bits per sample (8, 16, or 32)
 * 
 * @code
 * auto bits = audio_format_bit_size(audio_format::s16le);  // Returns 16
 * @endcode
 */
inline constexpr uint8_t audio_format_bit_size(audio_format fmt) {
    return static_cast<uint8_t>(static_cast<uint16_t>(fmt) & 0xFF);
}

/**
 * @brief Get byte size of audio format
 * @param fmt Audio format
 * @return Number of bytes per sample (1, 2, or 4)
 * 
 * @code
 * auto bytes = audio_format_byte_size(audio_format::f32le);  // Returns 4
 * @endcode
 */
inline constexpr uint8_t audio_format_byte_size(audio_format fmt) {
    return audio_format_bit_size(fmt) / 8;
}

/**
 * @brief Check if format uses signed samples
 * @param fmt Audio format
 * @return true if signed, false if unsigned
 * 
 * @note Only u8 format is unsigned, all others are signed
 */
inline constexpr bool audio_format_is_signed(audio_format fmt) {
    return (static_cast<uint16_t>(fmt) & 0x8000) != 0;
}

/**
 * @brief Check if format is big-endian
 * @param fmt Audio format
 * @return true if big-endian, false if little-endian
 * 
 * @note Most modern systems use little-endian
 */
inline constexpr bool audio_format_is_big_endian(audio_format fmt) {
    return (static_cast<uint16_t>(fmt) & 0x1000) != 0;
}

/**
 * @brief Check if format uses floating-point samples
 * @param fmt Audio format
 * @return true if floating-point, false if integer
 * 
 * @note Float formats provide better dynamic range but use more CPU
 */
inline constexpr bool audio_format_is_float(audio_format fmt) {
    return (static_cast<uint16_t>(fmt) & 0x0100) != 0;
}

/**
 * @defgroup native_formats Native System Formats
 * @brief Platform-specific default formats
 * 
 * These constants provide the native format for the current platform's
 * endianness, determined at compile time.
 * 
 * @{
 */

#if MUSAC_BIG_ENDIAN
inline constexpr audio_format audio_s16sys = audio_format::s16be;  ///< Native 16-bit signed
inline constexpr audio_format audio_s32sys = audio_format::s32be;  ///< Native 32-bit signed
inline constexpr audio_format audio_f32sys = audio_format::f32be;  ///< Native 32-bit float
#else
inline constexpr audio_format audio_s16sys = audio_format::s16le;  ///< Native 16-bit signed
inline constexpr audio_format audio_s32sys = audio_format::s32le;  ///< Native 32-bit signed
inline constexpr audio_format audio_f32sys = audio_format::f32le;  ///< Native 32-bit float
#endif

/** @} */ // end of native_formats

/**
 * @struct audio_spec
 * @brief Complete audio format specification
 * 
 * Combines format, channel count, and sample rate to fully describe
 * an audio stream's properties.
 * 
 * ## Common Configurations
 * 
 * @code
 * // CD quality audio
 * audio_spec cd_spec{
 *     .format = audio_format::s16le,
 *     .channels = 2,
 *     .freq = 44100
 * };
 * 
 * // Professional audio
 * audio_spec pro_spec{
 *     .format = audio_format::f32le,
 *     .channels = 2,
 *     .freq = 48000
 * };
 * 
 * // Voice/telephony
 * audio_spec voice_spec{
 *     .format = audio_format::s16le,
 *     .channels = 1,
 *     .freq = 8000
 * };
 * @endcode
 */
struct audio_spec {
    audio_format format;  ///< Sample format
    uint8_t channels;     ///< Number of channels (1=mono, 2=stereo, etc.)
    uint32_t freq;        ///< Sample rate in Hz
};

/**
 * @brief Stream output operator for audio_format
 * @param os Output stream
 * @param fmt Audio format to print
 * @return Reference to output stream
 * 
 * Prints human-readable format name (e.g., "s16le", "f32le")
 */
MUSAC_SDK_EXPORT std::ostream& operator<<(std::ostream& os, audio_format fmt);

/** @} */ // end of sdk_audio_format group

} // namespace musac

#endif // MUSAC_SDK_AUDIO_FORMAT_H