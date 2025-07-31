//
// Created by igor on 3/18/25.
//

#include <musac/sdk/samples_converter.hh>
#include <limits>
#include <type_traits>
#include <algorithm>


namespace musac {


    template<typename T>
    float as_float(T src) {
        static_assert(std::is_integral_v <T>, "T must be an integral type");
        constexpr double max_v = std::numeric_limits <T>::max();
        constexpr double min_v = std::numeric_limits <T>::min();
        constexpr double delta = max_v - min_v;
        constexpr auto scale = static_cast<float>(2.0 / delta);
        return static_cast <float>(static_cast <double>(src) - min_v) * scale - 1.0f;
    }

    void as_float_u8(float out[], const Uint8* buff, unsigned int samples) {
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(buff[i]);
        }
    }

    void as_float_s8(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const Sint8* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(data.buff[i]);
        }
    }

    void as_float_u16_le(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const Uint16* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            out[i] = as_float(data.buff[i]);
#else
            out[i] = as_float(SDL_Swap16(data.buff[i]));
#endif
        }
    }

    void as_float_u16_be(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const Uint16* buff;
        } data{};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            out[i] = as_float(SDL_Swap16(data.buff[i]));
#else
            out[i] = as_float(data.buff[i]);
#endif
        }
    }

    void as_float_s16_le(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const Sint16* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            out[i] = as_float(data.buff[i]);
#else
            out[i] = as_float(SDL_Swap16(data.buff[i]));
#endif
        }
    }

    void as_float_s16_be(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const Uint16* buff;
        } data{};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            out[i] = as_float(static_cast<Sint16>(SDL_Swap16(data.buff[i])));
#else
            out[i] = as_float(data.buff[i]);
#endif
        }
    }

    void as_float_s32_le(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const Sint32* buff;
        } data{};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            out[i] = as_float(data.buff[i]);
#else
            out[i] = as_float(SDL_Swap32(data.buff[i]));
#endif
        }
    }

    void as_float_s32_be(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const Uint32* buff;
        } data{};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            out[i] = as_float(static_cast<Sint32>(SDL_Swap32(data.buff[i])));
#else
            out[i] = as_float(data.buff[i]);
#endif
        }
    }

    void as_float_u32_le(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const Uint32* buff;
        } data{};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            out[i] = as_float(data.buff[i]);
#else
            out[i] = as_float(SDL_Swap32(data.buff[i]));
#endif
        }
    }

    void as_float_u32_be(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const Uint32* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            out[i] = as_float(SDL_Swap32(data.buff[i]));
#else
            out[i] = as_float(data.buff[i]);
#endif
        }
    }

    void as_float_f32_le(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const float* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            out[i] = data.buff[i];
#else
            out[i] = SDL_SwapFloat(data.buff[i]);
#endif
        }
    }

    void as_float_f32_be(float out[], const Uint8* buff, unsigned int samples) {
        union {
            const Uint8* x;
            const float* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            out[i] = SDL_SwapFloat(data.buff[i]);
#else
            out[i] = data.buff[i];
#endif
        }
    }

    to_float_converter_func_t get_to_float_conveter(SDL_AudioFormat format) {
        switch (format) {
            case SDL_AUDIO_S8:
                return as_float_s8;
            case SDL_AUDIO_U8:
                return as_float_u8;
            case SDL_AUDIO_S16LE:
                return as_float_s16_le;
            case SDL_AUDIO_S16BE:
                return as_float_s16_be;
            case SDL_AUDIO_S32LE:
                return as_float_f32_le;
            case SDL_AUDIO_S32BE:
                return as_float_f32_be;
            case SDL_AUDIO_F32LE:
                return as_float_f32_le;
            case SDL_AUDIO_F32BE:
                return as_float_f32_be;
            default:
                return nullptr;
        }
    }

    int bytes_per_sample(SDL_AudioFormat format) {
        switch (format) {
            case SDL_AUDIO_S8:
            case SDL_AUDIO_U8:
                return 1;
            case SDL_AUDIO_S16LE:
            case SDL_AUDIO_S16BE:
                return 2;
            default:
                return 4;
        }
    }
}
