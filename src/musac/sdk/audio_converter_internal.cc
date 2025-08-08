#include <musac/sdk/audio_converter_v2.hh>
#include <musac/sdk/endian.hh>
#include <musac/sdk/buffer.hh>
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstring>

namespace musac::detail {

// Catmull-Rom cubic interpolation helper
static float catmull_rom_interpolate(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    
    float a = -0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3;
    float b = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
    float c = -0.5f * p0 + 0.5f * p2;
    float d = p1;
    
    return a * t3 + b * t2 + c * t + d;
}

// Fast endian swap for 16-bit samples
void fast_swap16_inplace(uint8* data, size_t len) {
    uint16* samples = reinterpret_cast<uint16*>(data);
    size_t num_samples = len / sizeof(uint16);
    
    for (size_t i = 0; i < num_samples; ++i) {
        samples[i] = swap16be(samples[i]);
    }
}

// Fast endian swap for 32-bit samples
void fast_swap32_inplace(uint8* data, size_t len) {
    uint32* samples = reinterpret_cast<uint32*>(data);
    size_t num_samples = len / sizeof(uint32);
    
    for (size_t i = 0; i < num_samples; ++i) {
        samples[i] = swap32be(samples[i]);
    }
}

// Fast mono to stereo duplication
buffer<uint8> fast_mono_to_stereo(const uint8* data, size_t len, audio_format format) {
    size_t sample_size = audio_format_byte_size(format);
    size_t num_samples = len / sample_size;
    
    buffer<uint8> output(static_cast<unsigned int>(len * 2));
    
    if (sample_size == 1) {
        // 8-bit samples
        const uint8* src = data;
        uint8* dst = output.data();
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i * 2] = src[i];
            dst[i * 2 + 1] = src[i];
        }
    } else if (sample_size == 2) {
        // 16-bit samples
        const uint16* src = reinterpret_cast<const uint16*>(data);
        uint16* dst = reinterpret_cast<uint16*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i * 2] = src[i];
            dst[i * 2 + 1] = src[i];
        }
    } else if (sample_size == 4) {
        // 32-bit samples
        const uint32* src = reinterpret_cast<const uint32*>(data);
        uint32* dst = reinterpret_cast<uint32*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i * 2] = src[i];
            dst[i * 2 + 1] = src[i];
        }
    }
    
    return output;
}

// Fast stereo to mono averaging
buffer<uint8> fast_stereo_to_mono(const uint8* data, size_t len, audio_format format) {
    size_t sample_size = audio_format_byte_size(format);
    size_t num_frames = len / (sample_size * 2);
    
    buffer<uint8> output(static_cast<unsigned int>(len / 2));
    
    if (format == audio_format::u8) {
        const uint8* src = data;
        uint8* dst = output.data();
        for (size_t i = 0; i < num_frames; ++i) {
            dst[i] = (src[i * 2] + src[i * 2 + 1]) / 2;
        }
    } else if (format == audio_format::s16le || format == audio_format::s16be) {
        const int16* src = reinterpret_cast<const int16*>(data);
        int16* dst = reinterpret_cast<int16*>(output.data());
        for (size_t i = 0; i < num_frames; ++i) {
            dst[i] = (src[i * 2] + src[i * 2 + 1]) / 2;
        }
    } else if (format == audio_format::f32le || format == audio_format::f32be) {
        const float* src = reinterpret_cast<const float*>(data);
        float* dst = reinterpret_cast<float*>(output.data());
        for (size_t i = 0; i < num_frames; ++i) {
            dst[i] = (src[i * 2] + src[i * 2 + 1]) * 0.5f;
        }
    } else if (format == audio_format::s32le || format == audio_format::s32be) {
        const int32* src = reinterpret_cast<const int32*>(data);
        int32* dst = reinterpret_cast<int32*>(output.data());
        for (size_t i = 0; i < num_frames; ++i) {
            // Prevent overflow by dividing first
            dst[i] = src[i * 2] / 2 + src[i * 2 + 1] / 2;
        }
    }
    
    return output;
}

// Convert between sample formats (no channel/rate change)
buffer<uint8> convert_format(const uint8* data, size_t len, 
                             audio_format from, audio_format to, 
                             uint8 channels) {
    size_t from_size = audio_format_byte_size(from);
    size_t to_size = audio_format_byte_size(to);
    size_t num_samples = len / from_size;
    
    buffer<uint8> output(static_cast<unsigned int>(num_samples * to_size));
    
    // U8 to S16LE
    if (from == audio_format::u8 && to == audio_format::s16le) {
        const uint8* src = data;
        int16* dst = reinterpret_cast<int16*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = static_cast<int16>((src[i] - 128) << 8);
        }
    }
    // S8 to S16LE
    else if (from == audio_format::s8 && to == audio_format::s16le) {
        const int8* src = reinterpret_cast<const int8*>(data);
        int16* dst = reinterpret_cast<int16*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = static_cast<int16>(src[i] << 8);
        }
    }
    // S16LE to U8
    else if (from == audio_format::s16le && to == audio_format::u8) {
        const int16* src = reinterpret_cast<const int16*>(data);
        uint8* dst = output.data();
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = static_cast<uint8>((src[i] >> 8) + 128);
        }
    }
    // S16LE to S8
    else if (from == audio_format::s16le && to == audio_format::s8) {
        const int16* src = reinterpret_cast<const int16*>(data);
        int8* dst = reinterpret_cast<int8*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = static_cast<int8>(src[i] >> 8);
        }
    }
    // U8 to S8
    else if (from == audio_format::u8 && to == audio_format::s8) {
        const uint8* src = data;
        int8* dst = reinterpret_cast<int8*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = static_cast<int8>(src[i] - 128);
        }
    }
    // S8 to U8
    else if (from == audio_format::s8 && to == audio_format::u8) {
        const int8* src = reinterpret_cast<const int8*>(data);
        uint8* dst = output.data();
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = static_cast<uint8>(src[i] + 128);
        }
    }
    // S16LE to F32LE
    else if (from == audio_format::s16le && to == audio_format::f32le) {
        const int16* src = reinterpret_cast<const int16*>(data);
        float* dst = reinterpret_cast<float*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = src[i] / 32768.0f;
        }
    }
    // F32LE to S16LE
    else if (from == audio_format::f32le && to == audio_format::s16le) {
        const float* src = reinterpret_cast<const float*>(data);
        int16* dst = reinterpret_cast<int16*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            float val = src[i] * 32767.0f;
            val = std::max(-32768.0f, std::min(32767.0f, val));
            dst[i] = static_cast<int16>(val);
        }
    }
    // S16BE to S16LE (endian swap)
    else if (from == audio_format::s16be && to == audio_format::s16le) {
        const uint16* src = reinterpret_cast<const uint16*>(data);
        uint16* dst = reinterpret_cast<uint16*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = swap16be(src[i]);
        }
    }
    // S16LE to S16BE (endian swap)
    else if (from == audio_format::s16le && to == audio_format::s16be) {
        const uint16* src = reinterpret_cast<const uint16*>(data);
        uint16* dst = reinterpret_cast<uint16*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = swap16be(src[i]);
        }
    }
    // S32BE to S32LE (endian swap)
    else if (from == audio_format::s32be && to == audio_format::s32le) {
        const uint32* src = reinterpret_cast<const uint32*>(data);
        uint32* dst = reinterpret_cast<uint32*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = swap32be(src[i]);
        }
    }
    // S32LE to S32BE (endian swap)
    else if (from == audio_format::s32le && to == audio_format::s32be) {
        const uint32* src = reinterpret_cast<const uint32*>(data);
        uint32* dst = reinterpret_cast<uint32*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = swap32be(src[i]);
        }
    }
    // F32BE to F32LE (endian swap)
    else if (from == audio_format::f32be && to == audio_format::f32le) {
        const uint32* src = reinterpret_cast<const uint32*>(data);
        uint32* dst = reinterpret_cast<uint32*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = swap32be(src[i]);
        }
    }
    // F32LE to F32BE (endian swap)  
    else if (from == audio_format::f32le && to == audio_format::f32be) {
        const uint32* src = reinterpret_cast<const uint32*>(data);
        uint32* dst = reinterpret_cast<uint32*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = swap32be(src[i]);
        }
    }
    // S32LE to F32LE
    else if (from == audio_format::s32le && to == audio_format::f32le) {
        const int32* src = reinterpret_cast<const int32*>(data);
        float* dst = reinterpret_cast<float*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            dst[i] = src[i] / 2147483648.0f;
        }
    }
    // F32LE to S32LE
    else if (from == audio_format::f32le && to == audio_format::s32le) {
        const float* src = reinterpret_cast<const float*>(data);
        int32* dst = reinterpret_cast<int32*>(output.data());
        for (size_t i = 0; i < num_samples; ++i) {
            float val = src[i] * 2147483647.0f;
            val = std::max(-2147483648.0f, std::min(2147483647.0f, val));
            dst[i] = static_cast<int32>(val);
        }
    }
    // Same format - just copy
    else if (from == to) {
        std::memcpy(output.data(), data, len);
    }
    else {
        // Unsupported conversion - return empty buffer
        return buffer<uint8>(0);
    }
    
    return output;
}

// Resample audio data using cubic interpolation
buffer<uint8> resample_cubic(const uint8* data, size_t len,
                             audio_format format, uint8 channels,
                             uint32 src_freq, uint32 dst_freq) {
    if (src_freq == dst_freq) {
        // No resampling needed
        buffer<uint8> output(static_cast<unsigned int>(len));
        std::memcpy(output.data(), data, len);
        return output;
    }
    
    size_t sample_size = audio_format_byte_size(format);
    size_t frame_size = sample_size * channels;
    size_t src_frames = len / frame_size;
    size_t dst_frames = static_cast<size_t>(static_cast<double>(src_frames) * dst_freq / src_freq);
    
    buffer<uint8> output(static_cast<unsigned int>(dst_frames * frame_size));
    float ratio = static_cast<float>(src_frames) / static_cast<float>(dst_frames);
    
    // Convert to float for processing
    std::vector<float> src_float(src_frames * channels);
    std::vector<float> dst_float(dst_frames * channels);
    
    // Convert input to float
    if (format == audio_format::u8) {
        const uint8* src = data;
        for (size_t i = 0; i < src_frames * channels; ++i) {
            src_float[i] = (src[i] - 128) / 128.0f;
        }
    } else if (format == audio_format::s8) {
        const int8* src = reinterpret_cast<const int8*>(data);
        for (size_t i = 0; i < src_frames * channels; ++i) {
            src_float[i] = src[i] / 128.0f;
        }
    } else if (format == audio_format::s16le || format == audio_format::s16be) {
        const int16* src = reinterpret_cast<const int16*>(data);
        for (size_t i = 0; i < src_frames * channels; ++i) {
            int16 val = (format == audio_format::s16be) ? swap16be(src[i]) : src[i];
            src_float[i] = val / 32768.0f;
        }
    } else if (format == audio_format::f32le || format == audio_format::f32be) {
        const float* src = reinterpret_cast<const float*>(data);
        if (format == audio_format::f32be) {
            const uint32* src32 = reinterpret_cast<const uint32*>(data);
            for (size_t i = 0; i < src_frames * channels; ++i) {
                uint32 swapped = swap32be(src32[i]);
                src_float[i] = *reinterpret_cast<float*>(&swapped);
            }
        } else {
            std::memcpy(src_float.data(), src, src_frames * channels * sizeof(float));
        }
    }
    
    // Resample each channel
    for (uint8 ch = 0; ch < channels; ++ch) {
        for (size_t i = 0; i < dst_frames; ++i) {
            float src_pos = i * ratio;
            size_t src_idx = static_cast<size_t>(src_pos);
            float frac = src_pos - src_idx;
            
            // Get 4 points for cubic interpolation
            float p0, p1, p2, p3;
            
            if (src_idx > 0) {
                p0 = src_float[(src_idx - 1) * channels + ch];
            } else {
                p0 = src_float[ch];
            }
            
            p1 = src_float[src_idx * channels + ch];
            
            if (src_idx < src_frames - 1) {
                p2 = src_float[(src_idx + 1) * channels + ch];
            } else {
                p2 = src_float[(src_frames - 1) * channels + ch];
            }
            
            if (src_idx < src_frames - 2) {
                p3 = src_float[(src_idx + 2) * channels + ch];
            } else {
                p3 = src_float[(src_frames - 1) * channels + ch];
            }
            
            float interpolated = catmull_rom_interpolate(p0, p1, p2, p3, frac);
            interpolated = std::max(-1.0f, std::min(1.0f, interpolated));
            dst_float[i * channels + ch] = interpolated;
        }
    }
    
    // Convert back to target format
    if (format == audio_format::u8) {
        uint8* dst = output.data();
        for (size_t i = 0; i < dst_frames * channels; ++i) {
            dst[i] = static_cast<uint8>((dst_float[i] * 128.0f) + 128);
        }
    } else if (format == audio_format::s8) {
        int8* dst = reinterpret_cast<int8*>(output.data());
        for (size_t i = 0; i < dst_frames * channels; ++i) {
            dst[i] = static_cast<int8>(dst_float[i] * 127.0f);
        }
    } else if (format == audio_format::s16le || format == audio_format::s16be) {
        int16* dst = reinterpret_cast<int16*>(output.data());
        for (size_t i = 0; i < dst_frames * channels; ++i) {
            int16 val = static_cast<int16>(dst_float[i] * 32767.0f);
            dst[i] = (format == audio_format::s16be) ? swap16be(val) : val;
        }
    } else if (format == audio_format::f32le || format == audio_format::f32be) {
        if (format == audio_format::f32be) {
            uint32* dst32 = reinterpret_cast<uint32*>(output.data());
            for (size_t i = 0; i < dst_frames * channels; ++i) {
                float val = dst_float[i];
                uint32 raw = *reinterpret_cast<uint32*>(&val);
                dst32[i] = swap32be(raw);
            }
        } else {
            std::memcpy(output.data(), dst_float.data(), dst_frames * channels * sizeof(float));
        }
    }
    
    return output;
}

} // namespace musac::detail