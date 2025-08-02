#ifndef MUSAC_SDK_ENDIAN_H
#define MUSAC_SDK_ENDIAN_H

#include <musac/sdk/types.h>
#include <musac/sdk/musac_sdk_config.h>
#include <cstring>

namespace musac {

// Platform endianness detection using CMake-generated config
#if MUSAC_BIG_ENDIAN
    constexpr bool is_big_endian = true;
    constexpr bool is_little_endian = false;
#else
    constexpr bool is_big_endian = false;
    constexpr bool is_little_endian = true;
#endif

// Byte swapping functions
inline uint16 swap16(uint16 x) {
    return (x << 8) | (x >> 8);
}

inline uint32 swap32(uint32 x) {
    return ((x << 24) | ((x << 8) & 0x00FF0000) |
            ((x >> 8) & 0x0000FF00) | (x >> 24));
}

inline uint64 swap64(uint64 x) {
    return ((x << 56) |
            ((x << 40) & 0x00FF000000000000ULL) |
            ((x << 24) & 0x0000FF0000000000ULL) |
            ((x << 8)  & 0x000000FF00000000ULL) |
            ((x >> 8)  & 0x00000000FF000000ULL) |
            ((x >> 24) & 0x0000000000FF0000ULL) |
            ((x >> 40) & 0x000000000000FF00ULL) |
            (x >> 56));
}

inline float swap_float(float x) {
    union { float f; uint32 u; } data;
    data.f = x;
    data.u = swap32(data.u);
    return data.f;
}

// Conditional byte swapping based on platform
inline uint16 swap16le(uint16 x) {
    return is_little_endian ? x : swap16(x);
}

inline uint16 swap16be(uint16 x) {
    return is_big_endian ? x : swap16(x);
}

inline uint32 swap32le(uint32 x) {
    return is_little_endian ? x : swap32(x);
}

inline uint32 swap32be(uint32 x) {
    return is_big_endian ? x : swap32(x);
}

inline uint64 swap64le(uint64 x) {
    return is_little_endian ? x : swap64(x);
}

inline uint64 swap64be(uint64 x) {
    return is_big_endian ? x : swap64(x);
}

inline float swap_float_le(float x) {
    return is_little_endian ? x : swap_float(x);
}

inline float swap_float_be(float x) {
    return is_big_endian ? x : swap_float(x);
}

// Read macros for little-endian data
#define READ_16LE(ptr) musac::swap16le(*reinterpret_cast<const musac::uint16*>(ptr))
#define READ_32LE(ptr) musac::swap32le(*reinterpret_cast<const musac::uint32*>(ptr))

// Write macros for little-endian data
#define WRITE_16LE(ptr, val) (*reinterpret_cast<musac::uint16*>(ptr) = musac::swap16le(val))
#define WRITE_32LE(ptr, val) (*reinterpret_cast<musac::uint32*>(ptr) = musac::swap32le(val))

} // namespace musac

#endif // MUSAC_SDK_ENDIAN_H