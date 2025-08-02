#ifndef MUSAC_SDK_AUDIO_FORMAT_H
#define MUSAC_SDK_AUDIO_FORMAT_H

#include <musac/sdk/types.h>
#include <musac/sdk/musac_sdk_config.h>

namespace musac {

enum class audio_format : uint16 {
    unknown = 0,
    u8 = 0x0008,      // Unsigned 8-bit
    s8 = 0x8008,      // Signed 8-bit
    s16le = 0x8010,   // Signed 16-bit little-endian
    s16be = 0x9010,   // Signed 16-bit big-endian
    s32le = 0x8020,   // Signed 32-bit little-endian
    s32be = 0x9020,   // Signed 32-bit big-endian
    f32le = 0x8120,   // Float 32-bit little-endian
    f32be = 0x9120    // Float 32-bit big-endian
};

// Helper functions to work with audio formats
inline constexpr uint8 audio_format_bit_size(audio_format fmt) {
    return static_cast<uint8>(static_cast<uint16>(fmt) & 0xFF);
}

inline constexpr uint8 audio_format_byte_size(audio_format fmt) {
    return audio_format_bit_size(fmt) / 8;
}

inline constexpr bool audio_format_is_signed(audio_format fmt) {
    return (static_cast<uint16>(fmt) & 0x8000) != 0;
}

inline constexpr bool audio_format_is_big_endian(audio_format fmt) {
    return (static_cast<uint16>(fmt) & 0x1000) != 0;
}

inline constexpr bool audio_format_is_float(audio_format fmt) {
    return (static_cast<uint16>(fmt) & 0x0100) != 0;
}

// Platform-specific native format using CMake-generated config
#if MUSAC_BIG_ENDIAN
inline constexpr audio_format audio_s16sys = audio_format::s16be;
inline constexpr audio_format audio_s32sys = audio_format::s32be;
inline constexpr audio_format audio_f32sys = audio_format::f32be;
#else
inline constexpr audio_format audio_s16sys = audio_format::s16le;
inline constexpr audio_format audio_s32sys = audio_format::s32le;
inline constexpr audio_format audio_f32sys = audio_format::f32le;
#endif

struct audio_spec {
    audio_format format;
    uint8 channels;
    uint32 freq;
};

} // namespace musac

#endif // MUSAC_SDK_AUDIO_FORMAT_H