#include <doctest/doctest.h>
#include <musac/sdk/audio_converter.h>
#include <musac/sdk/audio_format.h>
#include <musac/sdk/samples_converter.hh>
#include <musac/sdk/memory.h>
#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>
#include <memory>
#include <limits>
#include <cstring>
#include <cstdio>

using musac::uint8;
using musac::int8;
using musac::uint16;
using musac::int16;
using musac::uint32;
using musac::int32;
using musac::int64;

namespace {
    // Helper function to create audio data for testing
    template<typename T>
    std::vector<uint8> create_test_data(const std::vector<T>& values) {
        std::vector<uint8> data(values.size() * sizeof(T));
        std::memcpy(data.data(), values.data(), data.size());
        return data;
    }
    
    // Helper function to extract values from byte data
    template<typename T>
    std::vector<T> extract_values(const musac::uint8* data, size_t count) {
        std::vector<T> values(count);
        std::memcpy(values.data(), data, count * sizeof(T));
        return values;
    }
    
    // Generate a sine wave for testing
    template<typename T>
    std::vector<T> generate_sine_wave(size_t samples, double frequency, double sample_rate, double amplitude = 1.0) {
        std::vector<T> wave(samples);
        const double two_pi = 2.0 * M_PI;
        
        for (size_t i = 0; i < samples; ++i) {
            double value = amplitude * std::sin(two_pi * frequency * static_cast<double>(i) / sample_rate);
            
            if constexpr (std::is_floating_point_v<T>) {
                wave[i] = static_cast<T>(value);
            } else if constexpr (std::is_same_v<T, int16_t>) {
                wave[i] = static_cast<T>(value * 32767.0);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                wave[i] = static_cast<T>(value * 2147483647.0);
            } else if constexpr (std::is_same_v<T, uint8_t>) {
                wave[i] = static_cast<T>((value + 1.0) * 127.5);
            } else if constexpr (std::is_same_v<T, int8_t>) {
                wave[i] = static_cast<T>(value * 127.0);
            }
        }
        
        return wave;
    }
    
    // Helper to check if two floating point values are approximately equal
    [[maybe_unused]] bool approx_equal(float a, float b, float epsilon = 0.001f) {
        return std::abs(a - b) < epsilon;
    }
}

TEST_SUITE("SDK::AudioConverter::Comprehensive") {
    TEST_CASE("Basic format conversions") {
        SUBCASE("U8 to S16LE") {
            musac::audio_spec src_spec = { musac::audio_format::u8, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 1, 44100 };
            
            std::vector<uint8_t> src_values = {0, 64, 128, 192, 255};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            CHECK(dst_data != nullptr);
            CHECK(dst_len == static_cast<int>(src_values.size() * 2));
            
            auto dst_values = extract_values<int16_t>(dst_data, src_values.size());
            
            // U8: 0-255 (128 = silence) -> S16: -32768 to 32767 (0 = silence)
            CHECK(dst_values[0] == -32768);
            CHECK(dst_values[1] == -16384);
            CHECK(dst_values[2] == 0);
            CHECK(dst_values[3] == 16384);
            CHECK(dst_values[4] == 32512);
            
            delete[] dst_data;
        }
        
        SUBCASE("S8 to S16LE") {
            musac::audio_spec src_spec = { musac::audio_format::s8, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 1, 44100 };
            
            std::vector<int8_t> src_values = {-128, -64, 0, 64, 127};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            auto dst_values = extract_values<int16_t>(dst_data, src_values.size());
            
            CHECK(dst_values[0] == -32768);
            CHECK(dst_values[1] == -16384);
            CHECK(dst_values[2] == 0);
            CHECK(dst_values[3] == 16384);
            CHECK(dst_values[4] == 32512);
            
            delete[] dst_data;
        }
        
        SUBCASE("S16LE to F32LE") {
            musac::audio_spec src_spec = { musac::audio_format::s16le, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::f32le, 1, 44100 };
            
            std::vector<int16_t> src_values = {-32768, -16384, 0, 16384, 32767};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            auto dst_values = extract_values<float>(dst_data, src_values.size());
            
            CHECK(approx_equal(dst_values[0], -1.0f));
            CHECK(approx_equal(dst_values[1], -0.5f));
            CHECK(approx_equal(dst_values[2], 0.0f));
            CHECK(approx_equal(dst_values[3], 0.5f));
            CHECK(approx_equal(dst_values[4], 1.0f, 0.0001f));
            
            delete[] dst_data;
        }
        
        SUBCASE("F32LE to S16LE") {
            musac::audio_spec src_spec = { musac::audio_format::f32le, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 1, 44100 };
            
            std::vector<float> src_values = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            auto dst_values = extract_values<int16_t>(dst_data, src_values.size());
            
            CHECK(dst_values[0] == -32768);
            CHECK(dst_values[1] == -16384);
            CHECK(dst_values[2] == 0);
            CHECK(dst_values[3] == 16384);
            CHECK(dst_values[4] == 32767);
            
            delete[] dst_data;
        }
        
        SUBCASE("S32LE to F32LE") {
            musac::audio_spec src_spec = { musac::audio_format::s32le, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::f32le, 1, 44100 };
            
            std::vector<int32_t> src_values = {INT32_MIN, INT32_MIN/2, 0, INT32_MAX/2, INT32_MAX};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            auto dst_values = extract_values<float>(dst_data, src_values.size());
            
            CHECK(approx_equal(dst_values[0], -1.0f));
            CHECK(approx_equal(dst_values[1], -0.5f));
            CHECK(approx_equal(dst_values[2], 0.0f));
            CHECK(approx_equal(dst_values[3], 0.5f, 0.0001f));
            CHECK(approx_equal(dst_values[4], 1.0f, 0.0001f));
            
            delete[] dst_data;
        }
    }
    
    TEST_CASE("Endianness conversions") {
        SUBCASE("S16LE to S16BE") {
            musac::audio_spec src_spec = { musac::audio_format::s16le, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s16be, 1, 44100 };
            
            std::vector<int16_t> src_values = {0x1234, static_cast<int16_t>(0xABCD)};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            // Check byte-swapped values
            CHECK(dst_data[0] == 0x34);
            CHECK(dst_data[1] == 0x12);
            CHECK(dst_data[2] == 0xCD);
            CHECK(dst_data[3] == 0xAB);
            
            delete[] dst_data;
        }
        
        SUBCASE("F32LE to F32BE") {
            musac::audio_spec src_spec = { musac::audio_format::f32le, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::f32be, 1, 44100 };
            
            std::vector<float> src_values = {1.0f, -1.0f};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            // Convert BE back to LE to verify
            musac::audio_spec verify_spec = { musac::audio_format::f32le, 1, 44100 };
            uint8* verify_data = nullptr;
            int verify_len = 0;
            
            CHECK(musac::convert_audio_samples(&dst_spec, dst_data, dst_len, 
                                              &verify_spec, &verify_data, &verify_len) == 1);
            
            auto verify_values = extract_values<float>(verify_data, src_values.size());
            CHECK(approx_equal(verify_values[0], 1.0f));
            CHECK(approx_equal(verify_values[1], -1.0f));
            
            delete[] dst_data;
            delete[] verify_data;
        }
    }
    
    TEST_CASE("Channel conversions") {
        SUBCASE("Mono to stereo") {
            musac::audio_spec src_spec = { musac::audio_format::s16le, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 2, 44100 };
            
            std::vector<int16_t> src_values = {1000, 2000, 3000, 4000};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            CHECK(dst_len == static_cast<int>(src_data.size() * 2));
            
            auto dst_values = extract_values<int16_t>(dst_data, src_values.size() * 2);
            
            // Check duplication
            CHECK(dst_values[0] == 1000);  // L1
            CHECK(dst_values[1] == 1000);  // R1
            CHECK(dst_values[2] == 2000);  // L2
            CHECK(dst_values[3] == 2000);  // R2
            CHECK(dst_values[4] == 3000);  // L3
            CHECK(dst_values[5] == 3000);  // R3
            CHECK(dst_values[6] == 4000);  // L4
            CHECK(dst_values[7] == 4000);  // R4
            
            delete[] dst_data;
        }
        
        SUBCASE("Stereo to mono") {
            musac::audio_spec src_spec = { musac::audio_format::s16le, 2, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 1, 44100 };
            
            std::vector<int16_t> src_values = {1000, 2000, 3000, 4000, -1000, 1000};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            CHECK(dst_len == static_cast<int>(src_data.size() / 2));
            
            auto dst_values = extract_values<int16_t>(dst_data, 3);
            
            CHECK(dst_values[0] == 1500);   // (1000 + 2000) / 2
            CHECK(dst_values[1] == 3500);   // (3000 + 4000) / 2
            CHECK(dst_values[2] == 0);      // (-1000 + 1000) / 2
            
            delete[] dst_data;
        }
        
        SUBCASE("Multi-channel conversions") {
            // 5.1 to stereo (should mix channels appropriately)
            musac::audio_spec src_spec = { musac::audio_format::s16le, 6, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 2, 44100 };
            
            // One frame: FL, FR, FC, LFE, BL, BR
            std::vector<int16_t> src_values = {1000, 2000, 3000, 500, 4000, 5000};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            delete[] dst_data;
        }
    }
    
    TEST_CASE("Sample rate conversions") {
        SUBCASE("Upsample 22050 to 44100") {
            musac::audio_spec src_spec = { musac::audio_format::f32le, 1, 22050 };
            musac::audio_spec dst_spec = { musac::audio_format::f32le, 1, 44100 };
            
            // Generate a 440Hz sine wave at 22050Hz
            auto src_values = generate_sine_wave<float>(100, 440.0, 22050.0);
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            // Should have approximately 2x samples
            CHECK(dst_len >= static_cast<int>(src_data.size() * 2 - 16)); // Allow some margin for resampling
            CHECK(dst_len <= static_cast<int>(src_data.size() * 2 + 16));
            
            delete[] dst_data;
        }
        
        SUBCASE("Downsample 48000 to 44100") {
            musac::audio_spec src_spec = { musac::audio_format::f32le, 1, 48000 };
            musac::audio_spec dst_spec = { musac::audio_format::f32le, 1, 44100 };
            
            // Generate a 440Hz sine wave at 48000Hz
            auto src_values = generate_sine_wave<float>(480, 440.0, 48000.0);
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            // Should have approximately 44100/48000 samples
            int expected_samples = static_cast<int>((src_values.size() * 44100) / 48000);
            int expected_len = static_cast<int>(static_cast<size_t>(expected_samples) * sizeof(float));
            CHECK(dst_len >= expected_len - 16);
            CHECK(dst_len <= expected_len + 16);
            
            delete[] dst_data;
        }
        
        SUBCASE("Non-integer ratio resampling") {
            musac::audio_spec src_spec = { musac::audio_format::f32le, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::f32le, 1, 48000 };
            
            auto src_values = generate_sine_wave<float>(441, 1000.0, 44100.0);
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            delete[] dst_data;
        }
    }
    
    TEST_CASE("Combined conversions") {
        SUBCASE("U8 mono 22050Hz to F32 stereo 44100Hz") {
            musac::audio_spec src_spec = { musac::audio_format::u8, 1, 22050 };
            musac::audio_spec dst_spec = { musac::audio_format::f32le, 2, 44100 };
            
            std::vector<uint8_t> src_values = {128, 160, 96, 128, 160, 96};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            // Should have approximately 2x samples (resampling) * 2 channels * 4 bytes
            CHECK(dst_len >= static_cast<int>(src_data.size() * 2 * 2 * 4 - 64));
            CHECK(dst_len <= static_cast<int>(src_data.size() * 2 * 2 * 4 + 64));
            
            delete[] dst_data;
        }
        
        SUBCASE("S16BE stereo 48000Hz to S16LE mono 44100Hz") {
            musac::audio_spec src_spec = { musac::audio_format::s16be, 2, 48000 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 1, 44100 };
            
            // Create test data with known byte-swapped values
            std::vector<uint8_t> src_data(16);
            // First stereo sample: 0x1234 (BE) for both channels
            src_data[0] = 0x12; src_data[1] = 0x34; // L
            src_data[2] = 0x12; src_data[3] = 0x34; // R
            // Second stereo sample: 0x5678 (BE) for both channels
            src_data[4] = 0x56; src_data[5] = 0x78; // L
            src_data[6] = 0x56; src_data[7] = 0x78; // R
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            delete[] dst_data;
        }
    }
    
    TEST_CASE("Edge cases") {
        SUBCASE("Zero-length input") {
            musac::audio_spec src_spec = { musac::audio_format::s16le, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::f32le, 1, 44100 };
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, nullptr, 0, 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            CHECK(dst_len == 0);
            
            if (dst_data) delete[] dst_data;
        }
        
        SUBCASE("Same format conversion (should be optimized)") {
            musac::audio_spec src_spec = { musac::audio_format::s16le, 2, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 2, 44100 };
            
            std::vector<int16_t> src_values = {1000, 2000, 3000, 4000};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            CHECK(dst_len == static_cast<int>(src_data.size()));
            
            // Should be a direct copy
            CHECK(std::memcmp(src_data.data(), dst_data, static_cast<size_t>(dst_len)) == 0);
            
            delete[] dst_data;
        }
        
        SUBCASE("Clipping behavior") {
            musac::audio_spec src_spec = { musac::audio_format::f32le, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 1, 44100 };
            
            // Values outside [-1, 1] range
            std::vector<float> src_values = {-2.0f, -1.5f, 1.5f, 2.0f, 10.0f, -10.0f};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            auto dst_values = extract_values<int16_t>(dst_data, src_values.size());
            
            // All should be clipped to valid range
            CHECK(dst_values[0] == -32768);
            CHECK(dst_values[1] == -32768);
            CHECK(dst_values[2] == 32767);
            CHECK(dst_values[3] == 32767);
            CHECK(dst_values[4] == 32767);
            CHECK(dst_values[5] == -32768);
            
            delete[] dst_data;
        }
        
        SUBCASE("Very high sample rate") {
            musac::audio_spec src_spec = { musac::audio_format::f32le, 1, 192000 };
            musac::audio_spec dst_spec = { musac::audio_format::f32le, 1, 44100 };
            
            // Just a few samples to test it works
            std::vector<float> src_values = {0.0f, 0.5f, 1.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.5f};
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            delete[] dst_data;
        }
        
        SUBCASE("Invalid format handling") {
            musac::audio_spec src_spec = { musac::audio_format::unknown, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 1, 44100 };
            
            uint8 dummy_data[10] = {0};
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, dummy_data, sizeof(dummy_data), 
                                              &dst_spec, &dst_data, &dst_len) == 0);
            CHECK(dst_data == nullptr);
        }
    }
    
    TEST_CASE("Performance optimizations") {
        SUBCASE("Same frequency float conversion using converters") {
            // When converting between float formats at same frequency,
            // we should be able to use to_float_converter and from_float_converter
            musac::audio_spec src_spec = { musac::audio_format::s16le, 1, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::f32le, 1, 44100 };
            
            auto src_values = generate_sine_wave<int16_t>(1000, 440.0, 44100.0);
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            // Verify using direct converter gives same result
            auto converter = musac::get_to_float_conveter(src_spec.format);
            CHECK(converter != nullptr);
            
            std::vector<float> direct_result(src_values.size());
            converter(direct_result.data(), src_data.data(), static_cast<unsigned int>(src_values.size()));
            
            auto converted_values = extract_values<float>(dst_data, src_values.size());
            
            for (size_t i = 0; i < src_values.size(); ++i) {
                CHECK(approx_equal(converted_values[i], direct_result[i]));
            }
            
            delete[] dst_data;
        }
        
        SUBCASE("Large buffer conversion") {
            musac::audio_spec src_spec = { musac::audio_format::s16le, 2, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::f32le, 2, 44100 };
            
            // 1 second of stereo audio
            const size_t samples = 44100 * 2;
            auto src_values = generate_sine_wave<int16_t>(samples, 440.0, 44100.0);
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            CHECK(dst_len == static_cast<int>(samples * sizeof(float)));
            
            delete[] dst_data;
        }
    }
    
    TEST_CASE("Real-world scenarios") {
        SUBCASE("CD quality to high-res") {
            // CD: 16-bit, 44.1kHz, stereo -> High-res: 24-bit (32-bit), 96kHz, stereo
            musac::audio_spec src_spec = { musac::audio_format::s16le, 2, 44100 };
            musac::audio_spec dst_spec = { musac::audio_format::s32le, 2, 96000 };
            
            // Generate a complex waveform (multiple frequencies)
            const size_t samples = 1000;
            std::vector<int16_t> src_values(samples);
            for (size_t i = 0; i < samples; ++i) {
                double t = static_cast<double>(i) / 44100.0;
                double value = 0.3 * std::sin(2 * M_PI * 440 * t) +    // A4
                              0.2 * std::sin(2 * M_PI * 554.37 * t) + // C#5
                              0.1 * std::sin(2 * M_PI * 659.25 * t);  // E5
                src_values[i] = static_cast<int16_t>(value * 32767);
            }
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            delete[] dst_data;
        }
        
        SUBCASE("Voice recording format conversion") {
            // Typical voice: 8-bit, 8kHz, mono -> Modern: 16-bit, 16kHz, mono
            musac::audio_spec src_spec = { musac::audio_format::u8, 1, 8000 };
            musac::audio_spec dst_spec = { musac::audio_format::s16le, 1, 16000 };
            
            // Simulate voice-like data
            std::vector<uint8_t> src_values(80); // 10ms at 8kHz
            for (size_t i = 0; i < src_values.size(); ++i) {
                src_values[i] = 128 + static_cast<uint8_t>(30 * std::sin(2 * M_PI * 200 * static_cast<double>(i) / 8000.0));
            }
            auto src_data = create_test_data(src_values);
            
            uint8* dst_data = nullptr;
            int dst_len = 0;
            
            CHECK(musac::convert_audio_samples(&src_spec, src_data.data(), static_cast<int>(src_data.size()), 
                                              &dst_spec, &dst_data, &dst_len) == 1);
            
            delete[] dst_data;
        }
    }
}