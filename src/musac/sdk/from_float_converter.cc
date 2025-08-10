//
// Created by igor on 4/28/25.
//

#include <cstring>
#include <limits>
#include <algorithm>
#include <musac/sdk/from_float_converter.hh>
#include <musac/sdk/endian.hh>
namespace musac {
    // Inline scale helper
    template<typename T>
    static T floatSampleToIntFast(const float f) noexcept {
        const float clamped = (f >= 1.f) ? 1.f : (f < -1.f ? -1.f : f);
        // scale by max value
        return static_cast<T>(static_cast<double>(clamped) * static_cast<double>(std::numeric_limits<T>::max()));
    }

    // S8: direct
    static void floatToS8(uint8_t* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t count = std::min((size_t)src.size(), dstBytes);
        auto* out = reinterpret_cast<int8_t*>(dst);
        for (size_t i = 0; i < count; ++i) {
            out[i] = floatSampleToIntFast<int8_t>(src[i]);
        }
        if (count < dstBytes) std::memset(dst + count, 0, dstBytes - count);
    }

    // U8: direct
    static void floatToU8(uint8_t* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t count = std::min((size_t)src.size(), dstBytes);
        uint8_t* out = dst;
        for (size_t i = 0; i < count; ++i) {
            out[i] = static_cast<uint8_t>((src[i] * 0.5f + 0.5f) * std::numeric_limits<uint8_t>::max());
        }
        if (count < dstBytes) std::memset(dst + count, 0x80, dstBytes - count);
    }

    // S16LE
    static void floatToS16LSB(uint8_t* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 2;
        size_t count = std::min((size_t)src.size(), maxSamples);
        int16_t* out = reinterpret_cast<int16_t*>(dst);
        for (size_t i = 0; i < count; ++i) {
            out[i] = floatSampleToIntFast<int16_t>(src[i]);
        }
        size_t used = count * 2;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // S16BE
    static void floatToS16MSB(uint8_t* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 2;
        size_t count = std::min((size_t)src.size(), maxSamples);
        uint8_t* p = dst;
        for (size_t i = 0; i < count; ++i) {
            auto v = static_cast<int16_t>(swap16(static_cast<uint16_t>(floatSampleToIntFast<int16_t>(src[i]))));
            *reinterpret_cast<int16_t*>(p) = v;
            p += 2;
        }
        size_t used = count * 2;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // S32LE
    static void floatToS32LSB(uint8_t* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        int32_t* out = reinterpret_cast<int32_t*>(dst);
        for (size_t i = 0; i < count; ++i) {
            out[i] = floatSampleToIntFast<int32_t>(src[i]);
        }
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // S32BE
    static void floatToS32MSB(uint8_t* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        uint8_t* p = dst;
        for (size_t i = 0; i < count; ++i) {
            auto v = static_cast<int32_t>(swap32(static_cast<uint32_t>(floatSampleToIntFast<int32_t>(src[i]))));
            *reinterpret_cast<int32_t*>(p) = v;
            p += 4;
        }
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // F32LE
    static void floatToFloatLSB(uint8_t* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        std::memcpy(dst, src.data(), count * 4);
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // F32BE
    static void floatToFloatMSB(uint8_t* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        uint8_t* p = dst;
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
