#ifndef MUSAC_SDK_SDL_COMPAT_H
#define MUSAC_SDK_SDL_COMPAT_H

// Temporary compatibility header for SDL migration
// This will be removed once all code is migrated

// Prevent SDL3 from being included after this header
#define SDL_h_ 1

#ifdef __cplusplus
#include <musac/sdk/types.h>
#include <musac/sdk/io_stream.h>
#include <musac/sdk/memory.h>
#include <musac/sdk/endian.h>
#include <vector>
#include <memory>
#else
// For C code, use standard C types
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#endif

// Type mappings
#ifdef __cplusplus
using Sint8 = musac::int8;
using Sint16 = musac::int16;
using Sint32 = musac::int32;
using Sint64 = musac::int64;
using Uint8 = musac::uint8;
using Uint16 = musac::uint16;
using Uint32 = musac::uint32;
using Uint64 = musac::uint64;
#else
typedef int8_t Sint8;
typedef int16_t Sint16;
typedef int32_t Sint32;
typedef int64_t Sint64;
typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
#endif

// SDL_IOStream mapping
#ifdef __cplusplus
using SDL_IOStream = musac::io_stream;
#else
// For C code, SDL_IOStream is an opaque pointer
typedef struct SDL_IOStream SDL_IOStream;
#endif

// I/O functions mapping
#ifdef __cplusplus
inline musac::size SDL_ReadIO(SDL_IOStream* context, void* ptr, musac::size size) {
    return context->read(ptr, size);
}

inline musac::size SDL_WriteIO(SDL_IOStream* context, const void* ptr, musac::size size) {
    return context->write(ptr, size);
}

inline musac::int64 SDL_SeekIO(SDL_IOStream* context, musac::int64 offset, int whence) {
    return context->seek(offset, static_cast<musac::seek_origin>(whence));
}

inline musac::int64 SDL_TellIO(SDL_IOStream* context) {
    return context->tell();
}

inline musac::int64 SDL_GetIOSize(SDL_IOStream* context) {
    return context->get_size();
}

inline void SDL_CloseIO(SDL_IOStream* context) {
    context->close();
}
#endif // __cplusplus

// Memory functions mapping
#ifdef __cplusplus
#define SDL_memcpy musac::memcpy
#define SDL_memset musac::memset
#define SDL_memmove musac::memmove
#define SDL_memcmp musac::memcmp
#define SDL_zero musac::zero
#define SDL_free(ptr) delete[] static_cast<Uint8*>(ptr)
#else
#define SDL_memcpy memcpy
#define SDL_memset memset
#define SDL_memmove memmove
#define SDL_memcmp memcmp
#define SDL_zero(x) memset(&(x), 0, sizeof(x))
#define SDL_free(ptr) free(ptr)
#endif

// Additional C string and memory functions
#ifndef __cplusplus
#define SDL_strncmp strncmp
#define SDL_calloc calloc
#define SDL_malloc malloc
#define SDL_strlcpy(dst, src, size) strncpy(dst, src, size)
#define SDL_snprintf snprintf
#define SDL_pow pow
#define SDL_log log
#define SDL_rand_bits() rand()
#define SDL_floor floor
#define SDL_fabs fabs
#define SDL_cos cos
#define SDL_sin sin
#define SDL_strlen strlen
#define SDL_strcmp strcmp
#define SDL_srand srand
#define SDL_GetPerformanceCounter() ((Uint64)time(NULL))
#define SDL_COMPILE_TIME_ASSERT(name, x) 
#endif

// Endian functions mapping
#ifdef __cplusplus
inline bool SDL_ReadU8(SDL_IOStream* src, Uint8* value) {
    return musac::read_u8(src, value);
}

inline bool SDL_ReadU16LE(SDL_IOStream* src, Uint16* value) {
    return musac::read_u16le(src, value);
}

inline bool SDL_ReadU16BE(SDL_IOStream* src, Uint16* value) {
    return musac::read_u16be(src, value);
}

inline bool SDL_ReadU32LE(SDL_IOStream* src, Uint32* value) {
    return musac::read_u32le(src, value);
}

inline bool SDL_ReadU32BE(SDL_IOStream* src, Uint32* value) {
    return musac::read_u32be(src, value);
}

inline bool SDL_ReadS16LE(SDL_IOStream* src, Sint16* value) {
    return musac::read_s16le(src, value);
}

inline bool SDL_ReadS16BE(SDL_IOStream* src, Sint16* value) {
    return musac::read_s16be(src, value);
}

inline bool SDL_ReadS32LE(SDL_IOStream* src, Sint32* value) {
    return musac::read_s32le(src, value);
}

inline bool SDL_ReadS32BE(SDL_IOStream* src, Sint32* value) {
    return musac::read_s32be(src, value);
}
#endif // __cplusplus

// Swap functions
#ifdef __cplusplus
#define SDL_Swap16 musac::swap16
#define SDL_Swap32 musac::swap32
#define SDL_SwapLE16 musac::swap16le
#define SDL_SwapBE16 musac::swap16be
#define SDL_SwapLE32 musac::swap32le
#define SDL_SwapBE32 musac::swap32be
#define SDL_Swap16BE musac::swap16be
#define SDL_Swap32BE musac::swap32be
#define SDL_SwapFloat musac::swap_float
#else
// For C code, provide simple inline implementations
static inline Uint16 SDL_Swap16(Uint16 x) {
    return ((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8);
}
static inline Uint32 SDL_Swap32(Uint32 x) {
    return ((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) |
           ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24);
}
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SDL_SwapLE16(x) SDL_Swap16(x)
#define SDL_SwapBE16(x) (x)
#define SDL_SwapLE32(x) SDL_Swap32(x)
#define SDL_SwapBE32(x) (x)
#define SDL_Swap16BE(x) (x)
#define SDL_Swap32BE(x) (x)
#else
#define SDL_SwapLE16(x) (x)
#define SDL_SwapBE16(x) SDL_Swap16(x)
#define SDL_SwapLE32(x) (x)
#define SDL_SwapBE32(x) SDL_Swap32(x)
#define SDL_Swap16BE(x) SDL_Swap16(x)
#define SDL_Swap32BE(x) SDL_Swap32(x)
#endif
#endif

// Additional swap functions with LE suffix (same as without suffix on little-endian systems)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SDL_Swap16LE(x) musac::swap16(x)
#define SDL_Swap32LE(x) musac::swap32(x)
#else
#define SDL_Swap16LE(x) (x)
#define SDL_Swap32LE(x) (x)
#endif

// SDL_bool type
#ifdef __cplusplus
using SDL_bool = bool;
#else
typedef int SDL_bool;
#endif
#define SDL_TRUE 1
#define SDL_FALSE 0

// Error handling
#ifdef __cplusplus
inline const char* SDL_GetError() {
    static const char* error_msg = "Operation failed";
    return error_msg;
}

inline void SDL_SetError(const char* fmt, ...) {
    // No-op for now
}

inline void SDL_ClearError() {
    // No-op for now
}
#else
static inline const char* SDL_GetError() {
    static const char* error_msg = "Operation failed";
    return error_msg;
}

static inline void SDL_SetError(const char* fmt, ...) {
    // No-op for now
}

static inline void SDL_ClearError() {
    // No-op for now
}
#endif

// Seek constants
#define SDL_IO_SEEK_SET 0
#define SDL_IO_SEEK_CUR 1
#define SDL_IO_SEEK_END 2

// SDL_IOWhence type
typedef int SDL_IOWhence;

// Audio format types
typedef Uint16 SDL_AudioFormat;

// SDL Audio format constants
#define SDL_AUDIO_UNKNOWN 0x0000  // Unknown audio format
#define SDL_AUDIO_U8     0x0008  // Unsigned 8-bit samples
#define SDL_AUDIO_S8     0x8008  // Signed 8-bit samples
#define SDL_AUDIO_S16LE  0x8010  // Signed 16-bit samples (little-endian)
#define SDL_AUDIO_S16BE  0x9010  // Signed 16-bit samples (big-endian)
#define SDL_AUDIO_S32LE  0x8020  // 32-bit integer samples (little-endian)
#define SDL_AUDIO_S32BE  0x9020  // 32-bit integer samples (big-endian)
#define SDL_AUDIO_F32LE  0x8120  // 32-bit floating point samples (little-endian)
#define SDL_AUDIO_F32BE  0x9120  // 32-bit floating point samples (big-endian)

// Platform-specific format aliases
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SDL_AUDIO_S16SYS SDL_AUDIO_S16BE
#define SDL_AUDIO_S32SYS SDL_AUDIO_S32BE
#define SDL_AUDIO_F32SYS SDL_AUDIO_F32BE
#define SDL_AUDIO_S16 SDL_AUDIO_S16BE
#define SDL_AUDIO_S32 SDL_AUDIO_S32BE
#define SDL_AUDIO_F32 SDL_AUDIO_F32BE
#else
#define SDL_AUDIO_S16SYS SDL_AUDIO_S16LE
#define SDL_AUDIO_S32SYS SDL_AUDIO_S32LE
#define SDL_AUDIO_F32SYS SDL_AUDIO_F32LE
#define SDL_AUDIO_S16 SDL_AUDIO_S16LE
#define SDL_AUDIO_S32 SDL_AUDIO_S32LE
#define SDL_AUDIO_F32 SDL_AUDIO_F32LE
#endif

// SDL_AudioSpec structure
struct SDL_AudioSpec {
    SDL_AudioFormat format;
    Uint8 channels;
    Uint32 freq;
};

// Audio format helpers
#define SDL_AUDIO_BITSIZE(x) (x & 0xFF)
#define SDL_AUDIO_BYTESIZE(x) (SDL_AUDIO_BITSIZE(x) / 8)
#define SDL_AUDIO_ISSIGNED(x) (x & 0x8000)
#define SDL_AUDIO_ISBIGENDIAN(x) (x & 0x1000)
#define SDL_AUDIO_ISFLOAT(x) (x & 0x100)

// Simple audio conversion (we'll implement this properly later)
#ifdef __cplusplus
inline int SDL_ConvertAudioSamples(const SDL_AudioSpec* src_spec, const Uint8* src_data, int src_len,
                                  const SDL_AudioSpec* dst_spec, Uint8** dst_data, int* dst_len) {
    // Calculate sample counts
    int src_sample_size = SDL_AUDIO_BYTESIZE(src_spec->format) * src_spec->channels;
    int dst_sample_size = SDL_AUDIO_BYTESIZE(dst_spec->format) * dst_spec->channels;
    int src_samples = src_len / src_sample_size;
    
    // Handle sample rate conversion
    int dst_samples = src_samples;
    if (src_spec->freq != dst_spec->freq) {
        dst_samples = (int)((Sint64)src_samples * dst_spec->freq / src_spec->freq);
    }
    
    // Allocate destination buffer
    *dst_len = dst_samples * dst_sample_size;
    *dst_data = new Uint8[*dst_len];
    
    // For now, handle simple cases
    if (src_spec->format == SDL_AUDIO_S16BE && dst_spec->format == SDL_AUDIO_S16LE &&
        src_spec->channels == dst_spec->channels && src_spec->freq == dst_spec->freq) {
        // Just byte swap
        const Uint16* src16 = (const Uint16*)src_data;
        Uint16* dst16 = (Uint16*)*dst_data;
        for (int i = 0; i < src_samples * src_spec->channels; i++) {
            dst16[i] = SDL_Swap16BE(src16[i]);
        }
        return 1; // Success
    }
    
    // Handle mono to mono conversion with different formats
    if (src_spec->channels == 1 && dst_spec->channels == 1) {
        // Convert U8 mono to S16LE mono (with possible sample rate conversion)
        if (src_spec->format == SDL_AUDIO_U8 && dst_spec->format == SDL_AUDIO_S16LE) {
            const Uint8* src8 = (const Uint8*)src_data;
            Sint16* dst16 = (Sint16*)*dst_data;
            
            if (src_spec->freq == dst_spec->freq) {
                // No resampling needed
                for (int i = 0; i < src_samples; i++) {
                    // Convert from unsigned 8-bit (0-255) to signed 16-bit
                    dst16[i] = ((Sint16)(src8[i] - 128)) << 8;
                }
            } else {
                // Cubic interpolation resampling for better quality
                float ratio = (float)src_samples / (float)dst_samples;
                for (int i = 0; i < dst_samples; i++) {
                    float src_pos = i * ratio;
                    int src_idx = (int)src_pos;
                    float frac = src_pos - src_idx;
                    
                    // Get 4 points for cubic interpolation
                    float p0, p1, p2, p3;
                    
                    // Handle boundary conditions
                    if (src_idx > 0) {
                        p0 = (float)(src8[src_idx - 1] - 128) / 128.0f;
                    } else {
                        p0 = (float)(src8[0] - 128) / 128.0f;
                    }
                    
                    p1 = (float)(src8[src_idx] - 128) / 128.0f;
                    
                    if (src_idx < src_samples - 1) {
                        p2 = (float)(src8[src_idx + 1] - 128) / 128.0f;
                    } else {
                        p2 = (float)(src8[src_samples - 1] - 128) / 128.0f;
                    }
                    
                    if (src_idx < src_samples - 2) {
                        p3 = (float)(src8[src_idx + 2] - 128) / 128.0f;
                    } else {
                        p3 = (float)(src8[src_samples - 1] - 128) / 128.0f;
                    }
                    
                    // Catmull-Rom cubic interpolation
                    float a = (-0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3);
                    float b = (p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3);
                    float c = (-0.5f * p0 + 0.5f * p2);
                    float d = p1;
                    
                    float frac2 = frac * frac;
                    float frac3 = frac2 * frac;
                    
                    float interpolated = a * frac3 + b * frac2 + c * frac + d;
                    
                    // Clamp to valid range
                    if (interpolated > 1.0f) interpolated = 1.0f;
                    if (interpolated < -1.0f) interpolated = -1.0f;
                    
                    dst16[i] = (Sint16)(interpolated * 32767.0f);
                }
            }
            return 1;
        }
        // Convert S16BE mono to S16LE mono (with possible sample rate conversion)
        if (src_spec->format == SDL_AUDIO_S16BE && dst_spec->format == SDL_AUDIO_S16LE) {
            const Uint16* src16 = (const Uint16*)src_data;
            Uint16* dst16 = (Uint16*)*dst_data;
            
            if (src_spec->freq == dst_spec->freq) {
                // No resampling needed
                for (int i = 0; i < src_samples; i++) {
                    dst16[i] = SDL_Swap16BE(src16[i]);
                }
            } else {
                // Cubic interpolation resampling with endian swap
                float ratio = (float)src_samples / (float)dst_samples;
                for (int i = 0; i < dst_samples; i++) {
                    float src_pos = i * ratio;
                    int src_idx = (int)src_pos;
                    float frac = src_pos - src_idx;
                    
                    // Get 4 points for cubic interpolation
                    float p0, p1, p2, p3;
                    
                    // Handle boundary conditions
                    if (src_idx > 0) {
                        p0 = (float)(Sint16)SDL_Swap16BE(src16[src_idx - 1]);
                    } else {
                        p0 = (float)(Sint16)SDL_Swap16BE(src16[0]);
                    }
                    
                    p1 = (float)(Sint16)SDL_Swap16BE(src16[src_idx]);
                    
                    if (src_idx < src_samples - 1) {
                        p2 = (float)(Sint16)SDL_Swap16BE(src16[src_idx + 1]);
                    } else {
                        p2 = (float)(Sint16)SDL_Swap16BE(src16[src_samples - 1]);
                    }
                    
                    if (src_idx < src_samples - 2) {
                        p3 = (float)(Sint16)SDL_Swap16BE(src16[src_idx + 2]);
                    } else {
                        p3 = (float)(Sint16)SDL_Swap16BE(src16[src_samples - 1]);
                    }
                    
                    // Catmull-Rom cubic interpolation
                    float a = (-0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3);
                    float b = (p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3);
                    float c = (-0.5f * p0 + 0.5f * p2);
                    float d = p1;
                    
                    float frac2 = frac * frac;
                    float frac3 = frac2 * frac;
                    
                    float interpolated = a * frac3 + b * frac2 + c * frac + d;
                    
                    // Clamp to valid range
                    if (interpolated > 32767.0f) interpolated = 32767.0f;
                    if (interpolated < -32768.0f) interpolated = -32768.0f;
                    
                    dst16[i] = (Sint16)interpolated;
                }
            }
            return 1;
        }
        // Convert S8 mono to S16LE mono (with possible sample rate conversion)
        if (src_spec->format == SDL_AUDIO_S8 && dst_spec->format == SDL_AUDIO_S16LE) {
            const Sint8* src8 = (const Sint8*)src_data;
            Sint16* dst16 = (Sint16*)*dst_data;
            
            if (src_spec->freq == dst_spec->freq) {
                // No resampling needed
                for (int i = 0; i < src_samples; i++) {
                    dst16[i] = ((Sint16)src8[i]) << 8;
                }
            } else {
                // Cubic interpolation resampling
                float ratio = (float)src_samples / (float)dst_samples;
                for (int i = 0; i < dst_samples; i++) {
                    float src_pos = i * ratio;
                    int src_idx = (int)src_pos;
                    float frac = src_pos - src_idx;
                    
                    // Get 4 points for cubic interpolation
                    float p0, p1, p2, p3;
                    
                    // Handle boundary conditions
                    if (src_idx > 0) {
                        p0 = (float)src8[src_idx - 1] / 128.0f;
                    } else {
                        p0 = (float)src8[0] / 128.0f;
                    }
                    
                    p1 = (float)src8[src_idx] / 128.0f;
                    
                    if (src_idx < src_samples - 1) {
                        p2 = (float)src8[src_idx + 1] / 128.0f;
                    } else {
                        p2 = (float)src8[src_samples - 1] / 128.0f;
                    }
                    
                    if (src_idx < src_samples - 2) {
                        p3 = (float)src8[src_idx + 2] / 128.0f;
                    } else {
                        p3 = (float)src8[src_samples - 1] / 128.0f;
                    }
                    
                    // Catmull-Rom cubic interpolation
                    float a = (-0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3);
                    float b = (p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3);
                    float c = (-0.5f * p0 + 0.5f * p2);
                    float d = p1;
                    
                    float frac2 = frac * frac;
                    float frac3 = frac2 * frac;
                    
                    float interpolated = a * frac3 + b * frac2 + c * frac + d;
                    
                    // Clamp to valid range
                    if (interpolated > 1.0f) interpolated = 1.0f;
                    if (interpolated < -1.0f) interpolated = -1.0f;
                    
                    dst16[i] = (Sint16)(interpolated * 32767.0f);
                }
            }
            return 1;
        }
    }
    
    // Handle stereo to mono conversion
    if (src_spec->channels == 2 && dst_spec->channels == 1) {
        // S16BE stereo to S16LE mono
        if (src_spec->format == SDL_AUDIO_S16BE && dst_spec->format == SDL_AUDIO_S16LE && 
            src_spec->freq == dst_spec->freq) {
            const Sint16* src16 = (const Sint16*)src_data;
            Sint16* dst16 = (Sint16*)*dst_data;
            for (int i = 0; i < src_samples; i++) {
                Sint32 left = (Sint16)SDL_Swap16BE(src16[i * 2]);
                Sint32 right = (Sint16)SDL_Swap16BE(src16[i * 2 + 1]);
                dst16[i] = (Sint16)((left + right) / 2);
            }
            *dst_len = src_samples * sizeof(Sint16);
            return 1;
        }
    }
    
    // Handle more mono conversions
    if (src_spec->channels == 1 && dst_spec->channels == 1) {
        // S16LE to S16LE with resampling
        if (src_spec->format == SDL_AUDIO_S16LE && dst_spec->format == SDL_AUDIO_S16LE &&
            src_spec->freq != dst_spec->freq) {
            const Sint16* src16 = (const Sint16*)src_data;
            Sint16* dst16 = (Sint16*)*dst_data;
            
            // Cubic interpolation resampling
            float ratio = (float)src_samples / (float)dst_samples;
            for (int i = 0; i < dst_samples; i++) {
                float src_pos = i * ratio;
                int src_idx = (int)src_pos;
                float frac = src_pos - src_idx;
                
                // Get 4 points for cubic interpolation
                float p0, p1, p2, p3;
                
                // Handle boundary conditions
                if (src_idx > 0) {
                    p0 = (float)src16[src_idx - 1];
                } else {
                    p0 = (float)src16[0];
                }
                
                p1 = (float)src16[src_idx];
                
                if (src_idx < src_samples - 1) {
                    p2 = (float)src16[src_idx + 1];
                } else {
                    p2 = (float)src16[src_samples - 1];
                }
                
                if (src_idx < src_samples - 2) {
                    p3 = (float)src16[src_idx + 2];
                } else {
                    p3 = (float)src16[src_samples - 1];
                }
                
                // Catmull-Rom cubic interpolation
                float a = (-0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3);
                float b = (p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3);
                float c = (-0.5f * p0 + 0.5f * p2);
                float d = p1;
                
                float frac2 = frac * frac;
                float frac3 = frac2 * frac;
                
                float interpolated = a * frac3 + b * frac2 + c * frac + d;
                
                // Clamp to valid range
                if (interpolated > 32767.0f) interpolated = 32767.0f;
                if (interpolated < -32768.0f) interpolated = -32768.0f;
                
                dst16[i] = (Sint16)interpolated;
            }
            return 1;
        }
        
        // S16 (native) to S16 (native) with resampling
        if (src_spec->format == SDL_AUDIO_S16 && dst_spec->format == SDL_AUDIO_S16 &&
            src_spec->freq != dst_spec->freq) {
            const Sint16* src16 = (const Sint16*)src_data;
            Sint16* dst16 = (Sint16*)*dst_data;
            
            // Cubic interpolation resampling
            float ratio = (float)src_samples / (float)dst_samples;
            for (int i = 0; i < dst_samples; i++) {
                float src_pos = i * ratio;
                int src_idx = (int)src_pos;
                float frac = src_pos - src_idx;
                
                // Get 4 points for cubic interpolation
                float p0, p1, p2, p3;
                
                // Handle boundary conditions
                if (src_idx > 0) {
                    p0 = (float)src16[src_idx - 1];
                } else {
                    p0 = (float)src16[0];
                }
                
                p1 = (float)src16[src_idx];
                
                if (src_idx < src_samples - 1) {
                    p2 = (float)src16[src_idx + 1];
                } else {
                    p2 = (float)src16[src_samples - 1];
                }
                
                if (src_idx < src_samples - 2) {
                    p3 = (float)src16[src_idx + 2];
                } else {
                    p3 = (float)src16[src_samples - 1];
                }
                
                // Catmull-Rom cubic interpolation
                float a = (-0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3);
                float b = (p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3);
                float c = (-0.5f * p0 + 0.5f * p2);
                float d = p1;
                
                float frac2 = frac * frac;
                float frac3 = frac2 * frac;
                
                float interpolated = a * frac3 + b * frac2 + c * frac + d;
                
                // Clamp to valid range
                if (interpolated > 32767.0f) interpolated = 32767.0f;
                if (interpolated < -32768.0f) interpolated = -32768.0f;
                
                dst16[i] = (Sint16)interpolated;
            }
            return 1;
        }
    }
    
    // Direct copy if formats match
    if (src_spec->format == dst_spec->format && 
        src_spec->channels == dst_spec->channels &&
        src_spec->freq == dst_spec->freq) {
        SDL_memcpy(*dst_data, src_data, src_len);
        *dst_len = src_len;
        return 1;
    }
    
    // Not implemented - but let's see what conversion was requested
    #ifdef DEBUG_SDL_COMPAT
    printf("SDL_ConvertAudioSamples: Unsupported conversion from format=%04X ch=%d freq=%d to format=%04X ch=%d freq=%d\n",
           src_spec->format, src_spec->channels, src_spec->freq,
           dst_spec->format, dst_spec->channels, dst_spec->freq);
    #endif
    
    // For now, allocate buffer and copy data unchanged if only sample rate differs
    if (src_spec->format == dst_spec->format && src_spec->channels == dst_spec->channels) {
        // Simple resampling - just duplicate/drop samples
        *dst_data = new Uint8[*dst_len];
        if (dst_samples >= src_samples) {
            // Upsample by duplicating
            int bytes_per_sample = SDL_AUDIO_BYTESIZE(src_spec->format) * src_spec->channels;
            for (int i = 0; i < dst_samples; i++) {
                int src_idx = (i * src_samples) / dst_samples;
                SDL_memcpy(*dst_data + i * bytes_per_sample, 
                          src_data + src_idx * bytes_per_sample, 
                          bytes_per_sample);
            }
        } else {
            // Downsample by dropping
            int bytes_per_sample = SDL_AUDIO_BYTESIZE(src_spec->format) * src_spec->channels;
            for (int i = 0; i < dst_samples; i++) {
                int src_idx = (i * src_samples) / dst_samples;
                SDL_memcpy(*dst_data + i * bytes_per_sample, 
                          src_data + src_idx * bytes_per_sample, 
                          bytes_per_sample);
            }
        }
        return 1;
    }
    
    delete[] *dst_data;
    *dst_data = nullptr;
    *dst_len = 0;
    return 0; // Conversion not implemented
}
#endif

// SDL_IOStream factory functions
#ifdef __cplusplus
// We need to manage these with unique_ptr internally
// but return raw pointers for SDL compatibility
inline SDL_IOStream* SDL_IOFromFile(const char* file, const char* mode) {
    static std::vector<std::unique_ptr<musac::io_stream>> managed_streams;
    auto stream = musac::io_from_file(file, mode);
    if (stream) {
        SDL_IOStream* raw = stream.get();
        managed_streams.push_back(std::move(stream));
        return raw;
    }
    return nullptr;
}

inline SDL_IOStream* SDL_IOFromConstMem(const void* mem, musac::size size) {
    static std::vector<std::unique_ptr<musac::io_stream>> managed_streams;
    auto stream = musac::io_from_memory(mem, size);
    if (stream) {
        SDL_IOStream* raw = stream.get();
        managed_streams.push_back(std::move(stream));
        return raw;
    }
    return nullptr;
}

inline SDL_IOStream* SDL_IOFromMem(void* mem, musac::size size) {
    static std::vector<std::unique_ptr<musac::io_stream>> managed_streams;
    auto stream = musac::io_from_memory_rw(mem, size);
    if (stream) {
        SDL_IOStream* raw = stream.get();
        managed_streams.push_back(std::move(stream));
        return raw;
    }
    return nullptr;
}
#endif // __cplusplus

#endif // MUSAC_SDK_SDL_COMPAT_H