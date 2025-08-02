#include <musac/sdk/audio_converter.h>
#include <musac/sdk/types.h>
#include <musac/sdk/endian.h>
#include <musac/sdk/memory.h>

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
    // Calculate sample counts
    int src_sample_size = audio_format_bytesize(src_spec->format) * src_spec->channels;
    int dst_sample_size = audio_format_bytesize(dst_spec->format) * dst_spec->channels;
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