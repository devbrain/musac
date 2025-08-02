#include <musac/sdk/audio_converter.h>
#include <musac/sdk/types.h>
#include <musac/sdk/endian.h>
#include <musac/sdk/memory.h>
#include <musac/sdk/samples_converter.hh>
#include <cmath>

namespace musac {

// Helper to get byte size from audio_format
inline int audio_format_bytesize(audio_format format) {
    switch (format) {
        case audio_format::u8:
        case audio_format::s8:
            return 1;
        case audio_format::s16le:
        case audio_format::s16be:
            return 2;
        case audio_format::s32le:
        case audio_format::s32be:
        case audio_format::f32le:
        case audio_format::f32be:
            return 4;
        default:
            return 0;
    }
}

int convert_audio_samples(const audio_spec* src_spec, const uint8* src_data, int src_len,
                         const audio_spec* dst_spec, uint8** dst_data, int* dst_len) {
    // Handle zero-length input
    if (src_len == 0 || src_data == nullptr) {
        *dst_len = 0;
        *dst_data = new uint8[1]; // Allocate minimal buffer
        return 1;
    }
    
    // Invalid format
    if (src_spec->format == audio_format::unknown || dst_spec->format == audio_format::unknown) {
        *dst_data = nullptr;
        *dst_len = 0;
        return 0;
    }
    
    // Calculate sample counts
    int src_sample_size = audio_format_bytesize(src_spec->format) * src_spec->channels;
    int dst_sample_size = audio_format_bytesize(dst_spec->format) * dst_spec->channels;
    
    if (src_sample_size == 0 || dst_sample_size == 0) {
        *dst_data = nullptr;
        *dst_len = 0;
        return 0;
    }
    
    int src_samples = src_len / src_sample_size;
    
    // Handle sample rate conversion
    int dst_samples = src_samples;
    if (src_spec->freq != dst_spec->freq) {
        dst_samples = (int)((int64)src_samples * dst_spec->freq / src_spec->freq);
    }
    
    // Allocate destination buffer
    *dst_len = dst_samples * dst_sample_size;
    *dst_data = new uint8[*dst_len];
    
    // For now, handle simple cases
    if (src_spec->format == audio_format::s16be && dst_spec->format == audio_format::s16le &&
        src_spec->channels == dst_spec->channels && src_spec->freq == dst_spec->freq) {
        // Just byte swap
        const uint16* src16 = (const uint16*)src_data;
        uint16* dst16 = (uint16*)*dst_data;
        for (int i = 0; i < src_samples * src_spec->channels; i++) {
            dst16[i] = swap16be(src16[i]);
        }
        return 1; // Success
    }
    
    // Handle mono to mono conversion with different formats
    if (src_spec->channels == 1 && dst_spec->channels == 1) {
        // Convert U8 mono to S16LE mono (with possible sample rate conversion)
        if (src_spec->format == audio_format::u8 && dst_spec->format == audio_format::s16le) {
            const uint8* src8 = (const uint8*)src_data;
            int16* dst16 = (int16*)*dst_data;
            
            if (src_spec->freq == dst_spec->freq) {
                // No resampling needed
                for (int i = 0; i < src_samples; i++) {
                    // Convert from unsigned 8-bit (0-255) to signed 16-bit
                    dst16[i] = ((int16)(src8[i] - 128)) << 8;
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
                    
                    dst16[i] = (int16)(interpolated * 32767.0f);
                }
            }
            return 1;
        }
        // Convert S16BE mono to S16LE mono (with possible sample rate conversion)
        if (src_spec->format == audio_format::s16be && dst_spec->format == audio_format::s16le) {
            const uint16* src16 = (const uint16*)src_data;
            uint16* dst16 = (uint16*)*dst_data;
            
            if (src_spec->freq == dst_spec->freq) {
                // No resampling needed
                for (int i = 0; i < src_samples; i++) {
                    dst16[i] = swap16be(src16[i]);
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
                        p0 = (float)(int16)swap16be(src16[src_idx - 1]);
                    } else {
                        p0 = (float)(int16)swap16be(src16[0]);
                    }
                    
                    p1 = (float)(int16)swap16be(src16[src_idx]);
                    
                    if (src_idx < src_samples - 1) {
                        p2 = (float)(int16)swap16be(src16[src_idx + 1]);
                    } else {
                        p2 = (float)(int16)swap16be(src16[src_samples - 1]);
                    }
                    
                    if (src_idx < src_samples - 2) {
                        p3 = (float)(int16)swap16be(src16[src_idx + 2]);
                    } else {
                        p3 = (float)(int16)swap16be(src16[src_samples - 1]);
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
                    
                    dst16[i] = (int16)interpolated;
                }
            }
            return 1;
        }
        // Convert S8 mono to S16LE mono (with possible sample rate conversion)
        if (src_spec->format == audio_format::s8 && dst_spec->format == audio_format::s16le) {
            const int8* src8 = (const int8*)src_data;
            int16* dst16 = (int16*)*dst_data;
            
            if (src_spec->freq == dst_spec->freq) {
                // No resampling needed
                for (int i = 0; i < src_samples; i++) {
                    dst16[i] = ((int16)src8[i]) << 8;
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
                    
                    dst16[i] = (int16)(interpolated * 32767.0f);
                }
            }
            return 1;
        }
    }
    
    // Handle stereo to mono conversion
    if (src_spec->channels == 2 && dst_spec->channels == 1) {
        // S16BE stereo to S16LE mono
        if (src_spec->format == audio_format::s16be && dst_spec->format == audio_format::s16le && 
            src_spec->freq == dst_spec->freq) {
            const int16* src16 = (const int16*)src_data;
            int16* dst16 = (int16*)*dst_data;
            for (int i = 0; i < src_samples; i++) {
                int32 left = (int16)swap16be(src16[i * 2]);
                int32 right = (int16)swap16be(src16[i * 2 + 1]);
                dst16[i] = (int16)((left + right) / 2);
            }
            *dst_len = src_samples * sizeof(int16);
            return 1;
        }
    }
    
    // Handle more mono conversions
    if (src_spec->channels == 1 && dst_spec->channels == 1) {
        // S16LE to S16LE with resampling
        if (src_spec->format == audio_format::s16le && dst_spec->format == audio_format::s16le &&
            src_spec->freq != dst_spec->freq) {
            const int16* src16 = (const int16*)src_data;
            int16* dst16 = (int16*)*dst_data;
            
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
                
                dst16[i] = (int16)interpolated;
            }
            return 1;
        }
        
        // S16 (native) to S16 (native) with resampling
        if (src_spec->format == audio_s16sys && dst_spec->format == audio_s16sys &&
            src_spec->freq != dst_spec->freq) {
            const int16* src16 = (const int16*)src_data;
            int16* dst16 = (int16*)*dst_data;
            
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
                
                dst16[i] = (int16)interpolated;
            }
            return 1;
        }
        
        // S16LE to F32LE conversion
        if (src_spec->format == audio_format::s16le && dst_spec->format == audio_format::f32le) {
            const int16* src16 = (const int16*)src_data;
            float* dstf32 = (float*)*dst_data;
            
            if (src_spec->freq == dst_spec->freq) {
                // No resampling needed - use optimized converter
                auto converter = get_to_float_conveter(src_spec->format);
                if (converter) {
                    converter(dstf32, src_data, src_samples);
                    return 1;
                }
                // Fallback if converter not available
                constexpr float scale = 1.0f / 32768.0f;
                for (int i = 0; i < src_samples; i++) {
                    dstf32[i] = static_cast<float>(src16[i]) * scale;
                }
            } else {
                // Resampling with format conversion
                float ratio = (float)src_samples / (float)dst_samples;
                constexpr float scale = 1.0f / 32768.0f;
                for (int i = 0; i < dst_samples; i++) {
                    float src_pos = i * ratio;
                    int src_idx = (int)src_pos;
                    float frac = src_pos - src_idx;
                    
                    // Cubic interpolation
                    float p0 = (src_idx > 0) ? src16[src_idx - 1] * scale : src16[0] * scale;
                    float p1 = src16[src_idx] * scale;
                    float p2 = (src_idx < src_samples - 1) ? src16[src_idx + 1] * scale : src16[src_samples - 1] * scale;
                    float p3 = (src_idx < src_samples - 2) ? src16[src_idx + 2] * scale : src16[src_samples - 1] * scale;
                    
                    float a = (-0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3);
                    float b = (p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3);
                    float c = (-0.5f * p0 + 0.5f * p2);
                    float d = p1;
                    
                    float frac2 = frac * frac;
                    float frac3 = frac2 * frac;
                    
                    dstf32[i] = a * frac3 + b * frac2 + c * frac + d;
                }
            }
            return 1;
        }
        
        // F32LE to S16LE conversion
        if (src_spec->format == audio_format::f32le && dst_spec->format == audio_format::s16le) {
            const float* srcf32 = (const float*)src_data;
            int16* dst16 = (int16*)*dst_data;
            
            if (src_spec->freq == dst_spec->freq) {
                // No resampling needed
                for (int i = 0; i < src_samples; i++) {
                    float value = srcf32[i];
                    // Clamp and convert
                    if (value >= 0.0f) {
                        value *= 32767.0f;
                        value = std::min(32767.0f, value);
                    } else {
                        value *= 32768.0f;
                        value = std::max(-32768.0f, value);
                    }
                    dst16[i] = static_cast<int16>(std::round(value));
                }
            } else {
                // Resampling with format conversion
                float ratio = (float)src_samples / (float)dst_samples;
                for (int i = 0; i < dst_samples; i++) {
                    float src_pos = i * ratio;
                    int src_idx = (int)src_pos;
                    float frac = src_pos - src_idx;
                    
                    // Cubic interpolation
                    float p0 = (src_idx > 0) ? srcf32[src_idx - 1] : srcf32[0];
                    float p1 = srcf32[src_idx];
                    float p2 = (src_idx < src_samples - 1) ? srcf32[src_idx + 1] : srcf32[src_samples - 1];
                    float p3 = (src_idx < src_samples - 2) ? srcf32[src_idx + 2] : srcf32[src_samples - 1];
                    
                    float a = (-0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3);
                    float b = (p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3);
                    float c = (-0.5f * p0 + 0.5f * p2);
                    float d = p1;
                    
                    float frac2 = frac * frac;
                    float frac3 = frac2 * frac;
                    
                    float value = a * frac3 + b * frac2 + c * frac + d;
                    
                    // Clamp and convert
                    if (value >= 0.0f) {
                        value *= 32767.0f;
                        value = std::min(32767.0f, value);
                    } else {
                        value *= 32768.0f;
                        value = std::max(-32768.0f, value);
                    }
                    dst16[i] = static_cast<int16>(std::round(value));
                }
            }
            return 1;
        }
        
        // S16LE to S32LE conversion
        if (src_spec->format == audio_format::s16le && dst_spec->format == audio_format::s32le) {
            const int16* src16 = (const int16*)src_data;
            int32* dst32 = (int32*)*dst_data;
            
            if (src_spec->freq == dst_spec->freq) {
                // No resampling needed
                for (int i = 0; i < src_samples; i++) {
                    dst32[i] = ((int32)src16[i]) << 16;
                }
            } else {
                // Resampling with format conversion
                float ratio = (float)src_samples / (float)dst_samples;
                for (int i = 0; i < dst_samples; i++) {
                    float src_pos = i * ratio;
                    int src_idx = (int)src_pos;
                    float frac = src_pos - src_idx;
                    
                    // Cubic interpolation
                    float p0 = (src_idx > 0) ? (float)src16[src_idx - 1] : (float)src16[0];
                    float p1 = (float)src16[src_idx];
                    float p2 = (src_idx < src_samples - 1) ? (float)src16[src_idx + 1] : (float)src16[src_samples - 1];
                    float p3 = (src_idx < src_samples - 2) ? (float)src16[src_idx + 2] : (float)src16[src_samples - 1];
                    
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
                    
                    dst32[i] = ((int32)interpolated) << 16;
                }
            }
            return 1;
        }
        
        // S32LE to F32LE conversion
        if (src_spec->format == audio_format::s32le && dst_spec->format == audio_format::f32le) {
            const int32* src32 = (const int32*)src_data;
            float* dstf32 = (float*)*dst_data;
            
            constexpr double scale = 1.0 / 2147483648.0;
            
            if (src_spec->freq == dst_spec->freq) {
                // No resampling needed
                for (int i = 0; i < src_samples; i++) {
                    dstf32[i] = static_cast<float>(static_cast<double>(src32[i]) * scale);
                }
            } else {
                // Resampling with format conversion
                float ratio = (float)src_samples / (float)dst_samples;
                for (int i = 0; i < dst_samples; i++) {
                    float src_pos = i * ratio;
                    int src_idx = (int)src_pos;
                    float frac = src_pos - src_idx;
                    
                    // Cubic interpolation
                    double p0 = (src_idx > 0) ? src32[src_idx - 1] * scale : src32[0] * scale;
                    double p1 = src32[src_idx] * scale;
                    double p2 = (src_idx < src_samples - 1) ? src32[src_idx + 1] * scale : src32[src_samples - 1] * scale;
                    double p3 = (src_idx < src_samples - 2) ? src32[src_idx + 2] * scale : src32[src_samples - 1] * scale;
                    
                    double a = (-0.5 * p0 + 1.5 * p1 - 1.5 * p2 + 0.5 * p3);
                    double b = (p0 - 2.5 * p1 + 2.0 * p2 - 0.5 * p3);
                    double c = (-0.5 * p0 + 0.5 * p2);
                    double d = p1;
                    
                    double frac2 = frac * frac;
                    double frac3 = frac2 * frac;
                    
                    dstf32[i] = static_cast<float>(a * frac3 + b * frac2 + c * frac + d);
                }
            }
            return 1;
        }
    }
    
    // Handle combined conversions (format + channel changes)
    // U8 mono to F32 stereo
    if (src_spec->format == audio_format::u8 && dst_spec->format == audio_format::f32le &&
        src_spec->channels == 1 && dst_spec->channels == 2) {
        const uint8* src8 = src_data;
        float* dstf32 = (float*)*dst_data;
        
        if (src_spec->freq == dst_spec->freq) {
            // No resampling, just format conversion and channel duplication
            for (int i = 0; i < src_samples; i++) {
                float value = ((float)(src8[i] - 128)) / 128.0f;
                dstf32[i * 2] = value;
                dstf32[i * 2 + 1] = value;
            }
        } else {
            // Resampling + format conversion + channel duplication
            float ratio = (float)src_samples / (float)dst_samples;
            for (int i = 0; i < dst_samples; i++) {
                float src_pos = i * ratio;
                int src_idx = (int)src_pos;
                float frac = src_pos - src_idx;
                
                // Cubic interpolation
                float p0 = (src_idx > 0) ? ((float)(src8[src_idx - 1] - 128)) / 128.0f : ((float)(src8[0] - 128)) / 128.0f;
                float p1 = ((float)(src8[src_idx] - 128)) / 128.0f;
                float p2 = (src_idx < src_samples - 1) ? ((float)(src8[src_idx + 1] - 128)) / 128.0f : ((float)(src8[src_samples - 1] - 128)) / 128.0f;
                float p3 = (src_idx < src_samples - 2) ? ((float)(src8[src_idx + 2] - 128)) / 128.0f : ((float)(src8[src_samples - 1] - 128)) / 128.0f;
                
                float a = (-0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3);
                float b = (p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3);
                float c = (-0.5f * p0 + 0.5f * p2);
                float d = p1;
                
                float frac2 = frac * frac;
                float frac3 = frac2 * frac;
                
                float value = a * frac3 + b * frac2 + c * frac + d;
                dstf32[i * 2] = value;
                dstf32[i * 2 + 1] = value;
            }
        }
        return 1;
    }
    
    // S16BE stereo to S16LE mono with resampling
    if (src_spec->format == audio_format::s16be && dst_spec->format == audio_format::s16le &&
        src_spec->channels == 2 && dst_spec->channels == 1) {
        const uint16* src16 = (const uint16*)src_data;
        int16* dst16 = (int16*)*dst_data;
        
        if (src_spec->freq == dst_spec->freq) {
            // No resampling
            for (int i = 0; i < dst_samples; i++) {
                int32 left = (int16)swap16be(src16[i * 2]);
                int32 right = (int16)swap16be(src16[i * 2 + 1]);
                dst16[i] = (int16)((left + right) / 2);
            }
        } else {
            // Resampling + endian conversion + channel mixing
            float ratio = (float)src_samples / (float)dst_samples;
            for (int i = 0; i < dst_samples; i++) {
                float src_pos = i * ratio;
                int src_idx = (int)src_pos;
                float frac = src_pos - src_idx;
                
                // Get stereo samples and convert to mono
                auto get_mono_sample = [&](int idx) -> float {
                    if (idx >= src_samples) idx = src_samples - 1;
                    int32 left = (int16)swap16be(src16[idx * 2]);
                    int32 right = (int16)swap16be(src16[idx * 2 + 1]);
                    return (float)((left + right) / 2);
                };
                
                // Cubic interpolation
                float p0 = (src_idx > 0) ? get_mono_sample(src_idx - 1) : get_mono_sample(0);
                float p1 = get_mono_sample(src_idx);
                float p2 = (src_idx < src_samples - 1) ? get_mono_sample(src_idx + 1) : get_mono_sample(src_samples - 1);
                float p3 = (src_idx < src_samples - 2) ? get_mono_sample(src_idx + 2) : get_mono_sample(src_samples - 1);
                
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
                
                dst16[i] = (int16)interpolated;
            }
        }
        return 1;
    }
    
    // Handle stereo format conversions
    if (src_spec->channels == 2 && dst_spec->channels == 2) {
        // S16LE stereo to F32LE stereo
        if (src_spec->format == audio_format::s16le && dst_spec->format == audio_format::f32le) {
            const int16* src16 = (const int16*)src_data;
            float* dstf32 = (float*)*dst_data;
            int total_samples = src_samples * 2; // stereo
            
            // Use optimized converter if available
            auto converter = get_to_float_conveter(src_spec->format);
            if (converter) {
                converter(dstf32, src_data, total_samples);
                return 1;
            }
            
            // Fallback
            constexpr float scale = 1.0f / 32768.0f;
            for (int i = 0; i < total_samples; i++) {
                dstf32[i] = static_cast<float>(src16[i]) * scale;
            }
            return 1;
        }
        
        // S16LE stereo to S32LE stereo  
        if (src_spec->format == audio_format::s16le && dst_spec->format == audio_format::s32le) {
            const int16* src16 = (const int16*)src_data;
            int32* dst32 = (int32*)*dst_data;
            
            if (src_spec->freq == dst_spec->freq) {
                int total_samples = src_samples * 2; // stereo
                for (int i = 0; i < total_samples; i++) {
                    dst32[i] = ((int32)src16[i]) << 16;
                }
            } else {
                // Resampling with format conversion for stereo
                float ratio = (float)src_samples / (float)dst_samples;
                for (int frame = 0; frame < dst_samples; frame++) {
                    float src_pos = frame * ratio;
                    int src_idx = (int)src_pos;
                    float frac = src_pos - src_idx;
                    
                    // Process both channels
                    for (int ch = 0; ch < 2; ch++) {
                        // Cubic interpolation
                        float p0 = (src_idx > 0) ? (float)src16[(src_idx - 1) * 2 + ch] : (float)src16[ch];
                        float p1 = (float)src16[src_idx * 2 + ch];
                        float p2 = (src_idx < src_samples - 1) ? (float)src16[(src_idx + 1) * 2 + ch] : (float)src16[(src_samples - 1) * 2 + ch];
                        float p3 = (src_idx < src_samples - 2) ? (float)src16[(src_idx + 2) * 2 + ch] : (float)src16[(src_samples - 1) * 2 + ch];
                        
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
                        
                        dst32[frame * 2 + ch] = ((int32)interpolated) << 16;
                    }
                }
            }
            return 1;
        }
    }
    
    // Handle channel conversions
    if (src_spec->format == dst_spec->format && src_spec->freq == dst_spec->freq) {
        // Mono to stereo
        if (src_spec->channels == 1 && dst_spec->channels == 2) {
            int bytes_per_sample = audio_format_bytesize(src_spec->format);
            const uint8* src = src_data;
            uint8* dst = *dst_data;
            
            for (int i = 0; i < src_samples; i++) {
                // Copy sample to both channels
                memcpy(dst + i * 2 * bytes_per_sample, src + i * bytes_per_sample, bytes_per_sample);
                memcpy(dst + (i * 2 + 1) * bytes_per_sample, src + i * bytes_per_sample, bytes_per_sample);
            }
            return 1;
        }
        
        // Stereo to mono (average channels)
        if (src_spec->channels == 2 && dst_spec->channels == 1) {
            if (src_spec->format == audio_format::s16le) {
                const int16* src16 = (const int16*)src_data;
                int16* dst16 = (int16*)*dst_data;
                for (int i = 0; i < dst_samples; i++) {
                    int32 left = src16[i * 2];
                    int32 right = src16[i * 2 + 1];
                    dst16[i] = (int16)((left + right) / 2);
                }
                return 1;
            } else if (src_spec->format == audio_format::f32le) {
                const float* srcf32 = (const float*)src_data;
                float* dstf32 = (float*)*dst_data;
                for (int i = 0; i < dst_samples; i++) {
                    dstf32[i] = (srcf32[i * 2] + srcf32[i * 2 + 1]) * 0.5f;
                }
                return 1;
            }
        }
        
        // Multi-channel to stereo (downmix)
        if (src_spec->channels > 2 && dst_spec->channels == 2) {
            if (src_spec->format == audio_format::s16le) {
                const int16* src16 = (const int16*)src_data;
                int16* dst16 = (int16*)*dst_data;
                
                for (int i = 0; i < dst_samples; i++) {
                    int32 left = 0, right = 0;
                    
                    if (src_spec->channels == 6) {
                        // 5.1: FL, FR, FC, LFE, BL, BR
                        int fl = src16[i * 6 + 0];
                        int fr = src16[i * 6 + 1];
                        int fc = src16[i * 6 + 2];
                        int lfe = src16[i * 6 + 3];
                        int bl = src16[i * 6 + 4];
                        int br = src16[i * 6 + 5];
                        
                        // Simple downmix formula
                        left = fl + (fc / 2) + (lfe / 4) + (bl / 2);
                        right = fr + (fc / 2) + (lfe / 4) + (br / 2);
                    } else {
                        // Generic downmix: take first two channels
                        left = src16[i * src_spec->channels + 0];
                        right = (src_spec->channels > 1) ? src16[i * src_spec->channels + 1] : left;
                    }
                    
                    // Clamp
                    if (left > 32767) left = 32767;
                    if (left < -32768) left = -32768;
                    if (right > 32767) right = 32767;
                    if (right < -32768) right = -32768;
                    
                    dst16[i * 2] = (int16)left;
                    dst16[i * 2 + 1] = (int16)right;
                }
                return 1;
            }
        }
    }
    
    // Handle endianness conversions at same sample rate
    if (src_spec->channels == dst_spec->channels && src_spec->freq == dst_spec->freq) {
        // S16LE to S16BE
        if (src_spec->format == audio_format::s16le && dst_spec->format == audio_format::s16be) {
            const uint16* src16 = (const uint16*)src_data;
            uint16* dst16 = (uint16*)*dst_data;
            int total_samples = src_samples * src_spec->channels;
            for (int i = 0; i < total_samples; i++) {
                dst16[i] = swap16le(src16[i]);
            }
            return 1;
        }
        
        // F32LE to F32BE
        if (src_spec->format == audio_format::f32le && dst_spec->format == audio_format::f32be) {
            const uint32* src32 = (const uint32*)src_data;
            uint32* dst32 = (uint32*)*dst_data;
            int total_samples = src_samples * src_spec->channels;
            for (int i = 0; i < total_samples; i++) {
                // Always swap for LE to BE conversion
                dst32[i] = swap32(src32[i]);
            }
            return 1;
        }
        
        // F32BE to F32LE
        if (src_spec->format == audio_format::f32be && dst_spec->format == audio_format::f32le) {
            const uint32* src32 = (const uint32*)src_data;
            uint32* dst32 = (uint32*)*dst_data;
            int total_samples = src_samples * src_spec->channels;
            for (int i = 0; i < total_samples; i++) {
                // Always swap for BE to LE conversion
                dst32[i] = swap32(src32[i]);
            }
            return 1;
        }
    }
    
    // Direct copy if formats match
    if (src_spec->format == dst_spec->format && 
        src_spec->channels == dst_spec->channels &&
        src_spec->freq == dst_spec->freq) {
        memcpy(*dst_data, src_data, src_len);
        *dst_len = src_len;
        return 1;
    }
    
    // Not implemented - but let's see what conversion was requested
    #ifdef DEBUG_SDL_COMPAT
    printf("convert_audio_samples: Unsupported conversion from format=%04X ch=%d freq=%d to format=%04X ch=%d freq=%d\n",
           static_cast<int>(src_spec->format), src_spec->channels, src_spec->freq,
           static_cast<int>(dst_spec->format), dst_spec->channels, dst_spec->freq);
    #endif
    
    // For now, allocate buffer and copy data unchanged if only sample rate differs
    if (src_spec->format == dst_spec->format && src_spec->channels == dst_spec->channels) {
        // Simple resampling - just duplicate/drop samples
        *dst_data = new uint8[*dst_len];
        if (dst_samples >= src_samples) {
            // Upsample by duplicating
            int bytes_per_sample = audio_format_bytesize(src_spec->format) * src_spec->channels;
            for (int i = 0; i < dst_samples; i++) {
                int src_idx = (i * src_samples) / dst_samples;
                memcpy(*dst_data + i * bytes_per_sample, 
                      src_data + src_idx * bytes_per_sample, 
                      bytes_per_sample);
            }
        } else {
            // Downsample by dropping
            int bytes_per_sample = audio_format_bytesize(src_spec->format) * src_spec->channels;
            for (int i = 0; i < dst_samples; i++) {
                int src_idx = (i * src_samples) / dst_samples;
                memcpy(*dst_data + i * bytes_per_sample, 
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

} // namespace musac