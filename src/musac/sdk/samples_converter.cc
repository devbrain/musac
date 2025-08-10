//
// Created by igor on 3/18/25.
//

#include <musac/sdk/samples_converter.hh>
#include <musac/sdk/endian.hh>
#include <limits>
#include <type_traits>
#include <algorithm>


namespace musac {


    template<typename T>
    float as_float(T src) {
        static_assert(std::is_integral_v <T>, "T must be an integral type");
        if constexpr (std::is_signed_v<T>) {
            // For signed types, normalize to [-1.0, 1.0]
            // Note: We add 1 to max to handle asymmetry (e.g., -128 to 127)
            constexpr double max_val = static_cast<double>(std::numeric_limits<T>::max()) + 1.0;
            return static_cast<float>(static_cast<double>(src) / max_val);
        } else {
            // For unsigned types, shift to signed range then normalize
            constexpr double max_v = std::numeric_limits<T>::max();
            constexpr double min_v = std::numeric_limits<T>::min();
            constexpr double delta = max_v - min_v;
            constexpr auto scale = static_cast<float>(2.0 / delta);
            return static_cast<float>(static_cast<double>(src) - min_v) * scale - 1.0f;
        }
    }

    void as_float_u8(float out[], const uint8_t* buff, unsigned int samples) {
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(buff[i]);
        }
    }

    void as_float_s8(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const int8_t* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(data.buff[i]);
        }
    }

    void as_float_u16_le(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const uint16_t* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(swap16le(data.buff[i]));
        }
    }

    void as_float_u16_be(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const uint16_t* buff;
        } data{};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(swap16be(data.buff[i]));
        }
    }

    void as_float_s16_le(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const int16_t* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(static_cast<int16_t>(swap16le(data.buff[i])));
        }
    }

    void as_float_s16_be(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const uint16_t* buff;
        } data{};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(static_cast<int16_t>(swap16be(data.buff[i])));
        }
    }

    void as_float_s32_le(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const int32_t* buff;
        } data{};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(static_cast<int32_t>(swap32le(data.buff[i])));
        }
    }

    void as_float_s32_be(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const uint32_t* buff;
        } data{};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(static_cast<int32_t>(swap32be(data.buff[i])));
        }
    }

    void as_float_u32_le(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const uint32_t* buff;
        } data{};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(swap32le(data.buff[i]));
        }
    }

    void as_float_u32_be(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const uint32_t* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = as_float(swap32be(data.buff[i]));
        }
    }

    void as_float_f32_le(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const float* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = swap_float_le(data.buff[i]);
        }
    }

    void as_float_f32_be(float out[], const uint8_t* buff, unsigned int samples) {
        union {
            const uint8_t* x;
            const float* buff;
        } data {};
        data.x = buff;
        for (unsigned int i = 0; i < samples; i++) {
            out[i] = swap_float_be(data.buff[i]);
        }
    }

    to_float_converter_func_t get_to_float_conveter(audio_format format) {
        switch (format) {
            case audio_format::s8:
                return as_float_s8;
            case audio_format::u8:
                return as_float_u8;
            case audio_format::s16le:
                return as_float_s16_le;
            case audio_format::s16be:
                return as_float_s16_be;
            case audio_format::s32le:
                return as_float_s32_le;
            case audio_format::s32be:
                return as_float_s32_be;
            case audio_format::f32le:
                return as_float_f32_le;
            case audio_format::f32be:
                return as_float_f32_be;
            default:
                return nullptr;
        }
    }

    int bytes_per_sample(audio_format format) {
        return audio_format_byte_size(format);
    }
}
