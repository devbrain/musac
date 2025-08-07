#include <doctest/doctest.h>
#include <cstdint>

// Placeholder audio format definitions for testing
// These will be replaced with actual implementations during SDL migration

enum class AudioFormat : uint16_t {
    Unknown = 0x0000,
    U8      = 0x0008,   // Unsigned 8-bit
    S8      = 0x8008,   // Signed 8-bit  
    S16LE   = 0x8010,   // Signed 16-bit little-endian
    S16BE   = 0x9010,   // Signed 16-bit big-endian
    S32LE   = 0x8020,   // Signed 32-bit little-endian
    S32BE   = 0x9020,   // Signed 32-bit big-endian
    F32LE   = 0x8120,   // Float 32-bit little-endian
    F32BE   = 0x9120    // Float 32-bit big-endian
};

// Helper functions for audio format properties
constexpr bool isAudioFormatSigned(AudioFormat fmt) {
    return (static_cast<uint16_t>(fmt) & 0x8000) != 0;
}

constexpr bool isAudioFormatBigEndian(AudioFormat fmt) {
    return (static_cast<uint16_t>(fmt) & 0x1000) != 0;
}

constexpr bool isAudioFormatFloat(AudioFormat fmt) {
    return (static_cast<uint16_t>(fmt) & 0x0100) != 0;
}

constexpr int getAudioFormatBitSize(AudioFormat fmt) {
    return static_cast<uint16_t>(fmt) & 0xFF;
}

constexpr int getAudioFormatByteSize(AudioFormat fmt) {
    return getAudioFormatBitSize(fmt) / 8;
}

struct AudioSpec {
    AudioFormat format;
    uint8_t channels;
    uint32_t freq;
};

TEST_SUITE("Core::AudioFormat") {
    TEST_CASE("Audio format properties") {
        SUBCASE("Bit size") {
            CHECK(getAudioFormatBitSize(AudioFormat::U8) == 8);
            CHECK(getAudioFormatBitSize(AudioFormat::S8) == 8);
            CHECK(getAudioFormatBitSize(AudioFormat::S16LE) == 16);
            CHECK(getAudioFormatBitSize(AudioFormat::S16BE) == 16);
            CHECK(getAudioFormatBitSize(AudioFormat::S32LE) == 32);
            CHECK(getAudioFormatBitSize(AudioFormat::S32BE) == 32);
            CHECK(getAudioFormatBitSize(AudioFormat::F32LE) == 32);
            CHECK(getAudioFormatBitSize(AudioFormat::F32BE) == 32);
        }
        
        SUBCASE("Byte size") {
            CHECK(getAudioFormatByteSize(AudioFormat::U8) == 1);
            CHECK(getAudioFormatByteSize(AudioFormat::S16LE) == 2);
            CHECK(getAudioFormatByteSize(AudioFormat::S32LE) == 4);
            CHECK(getAudioFormatByteSize(AudioFormat::F32LE) == 4);
        }
        
        SUBCASE("Signedness") {
            CHECK_FALSE(isAudioFormatSigned(AudioFormat::U8));
            CHECK(isAudioFormatSigned(AudioFormat::S8));
            CHECK(isAudioFormatSigned(AudioFormat::S16LE));
            CHECK(isAudioFormatSigned(AudioFormat::S16BE));
            CHECK(isAudioFormatSigned(AudioFormat::S32LE));
            CHECK(isAudioFormatSigned(AudioFormat::F32LE));
        }
        
        SUBCASE("Endianness") {
            CHECK_FALSE(isAudioFormatBigEndian(AudioFormat::S16LE));
            CHECK(isAudioFormatBigEndian(AudioFormat::S16BE));
            CHECK_FALSE(isAudioFormatBigEndian(AudioFormat::S32LE));
            CHECK(isAudioFormatBigEndian(AudioFormat::S32BE));
            CHECK_FALSE(isAudioFormatBigEndian(AudioFormat::F32LE));
            CHECK(isAudioFormatBigEndian(AudioFormat::F32BE));
        }
        
        SUBCASE("Float format") {
            CHECK_FALSE(isAudioFormatFloat(AudioFormat::U8));
            CHECK_FALSE(isAudioFormatFloat(AudioFormat::S16LE));
            CHECK_FALSE(isAudioFormatFloat(AudioFormat::S32LE));
            CHECK(isAudioFormatFloat(AudioFormat::F32LE));
            CHECK(isAudioFormatFloat(AudioFormat::F32BE));
        }
    }
    
    TEST_CASE("AudioSpec structure") {
        AudioSpec spec{AudioFormat::S16LE, 2, 44100};
        
        CHECK(spec.format == AudioFormat::S16LE);
        CHECK(spec.channels == 2);
        CHECK(spec.freq == 44100);
        
        // Calculate frame size
        int frame_size = getAudioFormatByteSize(spec.format) * spec.channels;
        CHECK(frame_size == 4); // 2 bytes * 2 channels
        
        // Calculate bytes per second
        int bytes_per_second = static_cast<int>(static_cast<uint32_t>(frame_size) * spec.freq);
        CHECK(bytes_per_second == 176400); // 4 * 44100
    }
    
    TEST_CASE("Sample value ranges") {
        SUBCASE("8-bit formats") {
            // U8: 0 to 255, silence at 128
            constexpr uint8_t u8_min = 0;
            constexpr uint8_t u8_max = 255;
            constexpr uint8_t u8_silence = 128;
            
            CHECK(u8_max - u8_min == 255);
            CHECK(u8_silence == 128);
            
            // S8: -128 to 127, silence at 0
            constexpr int8_t s8_min = -128;
            constexpr int8_t s8_max = 127;
            constexpr int8_t s8_silence = 0;
            
            CHECK(s8_max - s8_min == 255);
            CHECK(s8_silence == 0);
        }
        
        SUBCASE("16-bit formats") {
            // S16: -32768 to 32767, silence at 0
            constexpr int16_t s16_min = -32768;
            constexpr int16_t s16_max = 32767;
            constexpr int16_t s16_silence = 0;
            
            CHECK(s16_max - s16_min == 65535);
            CHECK(s16_silence == 0);
        }
        
        SUBCASE("32-bit formats") {
            // S32: -2147483648 to 2147483647, silence at 0
            constexpr int32_t s32_min = INT32_MIN;
            constexpr int32_t s32_max = INT32_MAX;
            constexpr int32_t s32_silence = 0;
            
            CHECK(s32_max > 0);
            CHECK(s32_min < 0);
            CHECK(s32_silence == 0);
            
            // F32: -1.0 to 1.0, silence at 0.0
            constexpr float f32_min = -1.0f;
            constexpr float f32_max = 1.0f;
            constexpr float f32_silence = 0.0f;
            
            CHECK(f32_max == 1.0f);
            CHECK(f32_min == -1.0f);
            CHECK(f32_silence == 0.0f);
        }
    }
}