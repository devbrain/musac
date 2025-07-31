/*
 * Type definitions for libmodplug without SDL3 dependency
 */

#ifndef _MODPLUG_TYPES_H_
#define _MODPLUG_TYPES_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

// Basic type definitions matching SDL3 types
typedef int8_t Sint8;
typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef int64_t Sint64;

// Byte swapping functions
#if defined(__GNUC__) || defined(__clang__)
  #define MODPLUG_BSWAP16(x) __builtin_bswap16(x)
  #define MODPLUG_BSWAP32(x) __builtin_bswap32(x)
#elif defined(_MSC_VER)
  #include <stdlib.h>
  #define MODPLUG_BSWAP16(x) _byteswap_ushort(x)
  #define MODPLUG_BSWAP32(x) _byteswap_ulong(x)
#else
  // Fallback implementations
  #define MODPLUG_BSWAP16(x) ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))
  #define MODPLUG_BSWAP32(x) ((((x) & 0xFF000000) >> 24) | \
                               (((x) & 0x00FF0000) >> 8) | \
                               (((x) & 0x0000FF00) << 8) | \
                               (((x) & 0x000000FF) << 24))
#endif

// Endian detection
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define MODPLUG_BIG_ENDIAN 1
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define MODPLUG_BIG_ENDIAN 0
#elif defined(_BIG_ENDIAN) || defined(__BIG_ENDIAN__)
  #define MODPLUG_BIG_ENDIAN 1
#elif defined(_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
  #define MODPLUG_BIG_ENDIAN 0
#elif defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) || defined(__arm__) || defined(__aarch64__)
  #define MODPLUG_BIG_ENDIAN 0
#else
  #error "Unable to determine endianness"
#endif

// SDL-style byte swapping macros
#if MODPLUG_BIG_ENDIAN
  #define SDL_Swap16LE(x) MODPLUG_BSWAP16(x)
  #define SDL_Swap32LE(x) MODPLUG_BSWAP32(x)
  #define SDL_Swap16BE(x) (x)
  #define SDL_Swap32BE(x) (x)
#else
  #define SDL_Swap16LE(x) (x)
  #define SDL_Swap32LE(x) (x)
  #define SDL_Swap16BE(x) MODPLUG_BSWAP16(x)
  #define SDL_Swap32BE(x) MODPLUG_BSWAP32(x)
#endif

// Memory and string function mappings
#define SDL_malloc malloc
#define SDL_calloc calloc
#define SDL_free free
#define SDL_memcpy memcpy
#define SDL_memset memset
#define SDL_memmove memmove
#define SDL_memcmp memcmp
#define SDL_strlen strlen
#define SDL_strcpy strcpy
#define SDL_strncpy strncpy
#define SDL_strcat strcat
#define SDL_strncat strncat
#define SDL_strcmp strcmp
#define SDL_strncmp strncmp
#define SDL_strchr strchr
#define SDL_strrchr strrchr
#define SDL_strstr strstr

// Math functions
#define SDL_fabs fabs
#define SDL_cos cos
#define SDL_sin sin
#define SDL_floor floor
#define SDL_pow pow
#define SDL_log log

// String functions
#define SDL_strlcpy(dst, src, size) do { \
    strncpy(dst, src, size - 1); \
    (dst)[size - 1] = '\0'; \
} while(0)

// Misc functions
#define SDL_zero(x) memset(&(x), 0, sizeof(x))
#define SDL_srand srand
#define SDL_GetPerformanceCounter() ((uint64_t)time(NULL))
#define SDL_rand_bits() rand()
#define SDL_snprintf snprintf

// Compile-time assertions
#define SDL_COMPILE_TIME_ASSERT(name, x) \
    typedef int SDL_compile_time_assert_##name[(x) * 2 - 1]

#endif /* _MODPLUG_TYPES_H_ */