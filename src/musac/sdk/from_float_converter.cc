//
// Created by igor on 4/28/25.
//

#include <cstring>
#include <limits>
#include <algorithm>
#include "../../../include/musac/sdk/from_float_converter.hh"

namespace musac {
    // Inline scale helper
    template<typename T>
    static T floatSampleToIntFast(const float f) noexcept {
        const float clamped = (f >= 1.f) ? 1.f : (f < -1.f ? -1.f : f);
        // scale by max value
        return static_cast<T>(static_cast<double>(clamped) * static_cast<double>(std::numeric_limits<T>::max()));
    }

    // S8: direct
    static void floatToS8(Uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t count = std::min((size_t)src.size(), dstBytes);
        auto* out = reinterpret_cast<Sint8*>(dst);
        for (size_t i = 0; i < count; ++i) {
            out[i] = floatSampleToIntFast<Sint8>(src[i]);
        }
        if (count < dstBytes) std::memset(dst + count, 0, dstBytes - count);
    }

    // U8: direct
    static void floatToU8(Uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t count = std::min((size_t)src.size(), dstBytes);
        Uint8* out = dst;
        for (size_t i = 0; i < count; ++i) {
            out[i] = static_cast<Uint8>((src[i] * 0.5f + 0.5f) * std::numeric_limits<Uint8>::max());
        }
        if (count < dstBytes) std::memset(dst + count, 0x80, dstBytes - count);
    }

    // S16LE
    static void floatToS16LSB(Uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 2;
        size_t count = std::min((size_t)src.size(), maxSamples);
        Sint16* out = reinterpret_cast<Sint16*>(dst);
        for (size_t i = 0; i < count; ++i) {
            out[i] = floatSampleToIntFast<Sint16>(src[i]);
        }
        size_t used = count * 2;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // S16BE
    static void floatToS16MSB(Uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 2;
        size_t count = std::min((size_t)src.size(), maxSamples);
        Uint8* p = dst;
        for (size_t i = 0; i < count; ++i) {
            auto v = static_cast<Sint16>(SDL_Swap16(static_cast<Uint16>(floatSampleToIntFast<Sint16>(src[i]))));
            *reinterpret_cast<Sint16*>(p) = v;
            p += 2;
        }
        size_t used = count * 2;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // S32LE
    static void floatToS32LSB(Uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        Sint32* out = reinterpret_cast<Sint32*>(dst);
        for (size_t i = 0; i < count; ++i) {
            out[i] = floatSampleToIntFast<Sint32>(src[i]);
        }
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // S32BE
    static void floatToS32MSB(Uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        Uint8* p = dst;
        for (size_t i = 0; i < count; ++i) {
            auto v = static_cast<Sint32>(SDL_Swap32(static_cast<Uint32>(floatSampleToIntFast<Sint32>(src[i]))));
            *reinterpret_cast<Sint32*>(p) = v;
            p += 4;
        }
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // F32LE
    static void floatToFloatLSB(Uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        std::memcpy(dst, src.data(), count * 4);
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // F32BE
    static void floatToFloatMSB(Uint8* dst, size_t dstBytes, const buffer<float>& src) noexcept {
        size_t maxSamples = dstBytes / 4;
        size_t count = std::min((size_t)src.size(), maxSamples);
        Uint8* p = dst;
        for (size_t i = 0; i < count; ++i) {
            float v = SDL_SwapFloat(src[i]);
            *reinterpret_cast<float*>(p) = v;
            p += 4;
        }
        size_t used = count * 4;
        if (used < dstBytes) std::memset(dst + used, 0, dstBytes - used);
    }

    // Lookup converter
    from_float_converter_func_t get_from_float_converter(SDL_AudioFormat fmt) {
        switch (fmt) {
            case SDL_AUDIO_S8:     return floatToS8;
            case SDL_AUDIO_U8:     return floatToU8;
            case SDL_AUDIO_S16LE:  return floatToS16LSB;
            case SDL_AUDIO_S16BE:  return floatToS16MSB;
            case SDL_AUDIO_S32LE:  return floatToS32LSB;
            case SDL_AUDIO_S32BE:  return floatToS32MSB;
            case SDL_AUDIO_F32LE:  return floatToFloatLSB;
            case SDL_AUDIO_F32BE:  return floatToFloatMSB;
            default:               return nullptr;
        }
    }
}
