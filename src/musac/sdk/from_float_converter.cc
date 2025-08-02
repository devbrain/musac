//
// Created by igor on 4/28/25.
//

#include <cstring>
#include <limits>
#include <algorithm>
#include <musac/sdk/from_float_converter.hh>
#include <musac/sdk/endian.h>
namespace musac {
    // Inline scale helper
    template<typename T>
    static T floatSampleToIntFast(const float f) noexcept {
        const float clamped = (f >= 1.f) ? 1.f : (f < -1.f ? -1.f : f);
        // scale by max value
        return static_cast<T>(static_cast<double>(clamped) * static_cast<double>(std::numeric_limits<T>::max()));
    }

    // S8: direct
    static void floatToS8(uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t count = std::min((size_t)src.size(), dstBytes);
        auto* out = reinterpret_cast<int8*>(dst);
        for (size_t i = 0; i < count; ++i) {
            out[i] = floatSampleToIntFast<int8>(src[i]);
        }
        if (count < dstBytes) std::memset(dst + count, 0, dstBytes - count);
    }

    // U8: direct
    static void floatToU8(uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t count = std::min((size_t)src.size(), dstBytes);
        uint8* out = dst;
        for (size_t i = 0; i < count; ++i) {
            out[i] = static_cast<uint8>((src[i] * 0.5f + 0.5f) * std::numeric_limits<uint8>::max());
        }
        if (count < dstBytes) std::memset(dst + count, 0x80, dstBytes - count);
    }

    // S16LE
    static void floatToS16LSB(uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 2;
        size_t count = std::min((size_t)src.size(), maxSamples);
        int16* out = reinterpret_cast<int16*>(dst);
        for (size_t i = 0; i < count; ++i) {
            out[i] = floatSampleToIntFast<int16>(src[i]);
        }
        size_t used = count * 2;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // S16BE
    static void floatToS16MSB(uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 2;
        size_t count = std::min((size_t)src.size(), maxSamples);
        uint8* p = dst;
        for (size_t i = 0; i < count; ++i) {
            auto v = static_cast<int16>(swap16(static_cast<uint16>(floatSampleToIntFast<int16>(src[i]))));
            *reinterpret_cast<int16*>(p) = v;
            p += 2;
        }
        size_t used = count * 2;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // S32LE
    static void floatToS32LSB(uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        int32* out = reinterpret_cast<int32*>(dst);
        for (size_t i = 0; i < count; ++i) {
            out[i] = floatSampleToIntFast<int32>(src[i]);
        }
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // S32BE
    static void floatToS32MSB(uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        uint8* p = dst;
        for (size_t i = 0; i < count; ++i) {
            auto v = static_cast<int32>(swap32(static_cast<uint32>(floatSampleToIntFast<int32>(src[i]))));
            *reinterpret_cast<int32*>(p) = v;
            p += 4;
        }
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // F32LE
    static void floatToFloatLSB(uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        std::memcpy(dst, src.data(), count * 4);
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // F32BE
    static void floatToFloatMSB(uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        uint8* p = dst;
        for (size_t i = 0; i < count; ++i) {
            float v = swap_float(src[i]);
            *reinterpret_cast<float*>(p) = v;
            p += 4;
        }
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // Lookup converter
    from_float_converter_func_t get_from_float_converter(audio_format fmt) {
        switch (fmt) {
            case audio_format::s8:     return floatToS8;
            case audio_format::u8:     return floatToU8;
            case audio_format::s16le:  return floatToS16LSB;
            case audio_format::s16be:  return floatToS16MSB;
            case audio_format::s32le:  return floatToS32LSB;
            case audio_format::s32be:  return floatToS32MSB;
            case audio_format::f32le:  return floatToFloatLSB;
            case audio_format::f32be:  return floatToFloatMSB;
            default:                   return nullptr;
        }
    }
}
