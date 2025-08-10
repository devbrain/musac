#ifndef MUSAC_SDK_ENDIAN_H
#define MUSAC_SDK_ENDIAN_H

#include <musac/sdk/types.hh>
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
inline uint16_t swap16(uint16_t x) {
    return (x << 8) | (x >> 8);
}

inline uint32_t swap32(uint32_t x) {
    return ((x << 24) | ((x << 8) & 0x00FF0000) |
            ((x >> 8) & 0x0000FF00) | (x >> 24));
}

inline uint64_t swap64(uint64_t x) {
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
    union { float f; uint32_t u; } data;
    data.f = x;
    data.u = swap32(data.u);
    return data.f;
}

// Conditional byte swapping based on platform
inline uint16_t swap16le(uint16_t x) {
    return is_little_endian ? x : swap16(x);
}

inline uint16_t swap16be(uint16_t x) {
    return is_big_endian ? x : swap16(x);
}

inline uint32_t swap32le(uint32_t x) {
    return is_little_endian ? x : swap32(x);
}

inline uint32_t swap32be(uint32_t x) {
    return is_big_endian ? x : swap32(x);
}

inline uint64_t swap64le(uint64_t x) {
    return is_little_endian ? x : swap64(x);
}

inline uint64_t swap64be(uint64_t x) {
    return is_big_endian ? x : swap64(x);
}

inline float swap_float_le(float x) {
    return is_little_endian ? x : swap_float(x);
}

inline float swap_float_be(float x) {
    return is_big_endian ? x : swap_float(x);
}

// Read macros for little-endian data
#define READ_16LE(ptr) musac::swap16le(*reinterpret_cast<const musac::uint16_t*>(ptr))
#define READ_32LE(ptr) musac::swap32le(*reinterpret_cast<const musac::uint32_t*>(ptr))

// Write macros for little-endian data
#define WRITE_16LE(ptr, val) (*reinterpret_cast<musac::uint16_t*>(ptr) = musac::swap16le(val))
#define WRITE_32LE(ptr, val) (*reinterpret_cast<musac::uint32_t*>(ptr) = musac::swap32le(val))

} // namespace musac

#endif // MUSAC_SDK_ENDIAN_H