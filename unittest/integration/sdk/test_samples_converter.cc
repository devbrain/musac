#include <doctest/doctest.h>
#include <musac/sdk/samples_converter.hh>
#include <musac/sdk/audio_format.hh>
#include <vector>
#include <cmath>
#include <algorithm>

TEST_SUITE("SDK::SamplesConverter") {
    TEST_CASE("to_float_converter function selection") {
        // Test that we get the correct converter for each format
        auto converter_u8 = musac::get_to_float_conveter(musac::audio_format::u8);
        auto converter_s16 = musac::get_to_float_conveter(musac::audio_s16sys);
        auto converter_s32 = musac::get_to_float_conveter(musac::audio_s32sys);
        auto converter_f32 = musac::get_to_float_conveter(musac::audio_f32sys);
        
        CHECK(converter_u8 != nullptr);
        CHECK(converter_s16 != nullptr);
        CHECK(converter_s32 != nullptr);
        CHECK(converter_f32 != nullptr);
        
        // Different formats should have different converters
        CHECK(converter_u8 != converter_s16);
        CHECK(converter_s16 != converter_s32);
        CHECK(converter_s32 != converter_f32);
    }
    
    TEST_CASE("U8 to float conversion") {
        auto converter = musac::get_to_float_conveter(musac::audio_format::u8);
        REQUIRE(converter != nullptr);
        
        SUBCASE("Basic conversion") {
            uint8_t src[] = {0, 64, 128, 192, 255};
            float dst[5];
            
            converter(dst, reinterpret_cast<const uint8_t*>(src), 5);
            
            // U8: 0-255, 128 = silence
            // Float: -1.0 to 1.0, 0.0 = silence
            CHECK(dst[0] == doctest::Approx(-1.0f).epsilon(0.01f));
            CHECK(dst[1] == doctest::Approx(-0.5f).epsilon(0.01f));
            CHECK(dst[2] == doctest::Approx(0.0f).epsilon(0.01f));
            CHECK(dst[3] == doctest::Approx(0.5f).epsilon(0.01f));
            CHECK(dst[4] == doctest::Approx(1.0f).epsilon(0.01f));
        }
        
        SUBCASE("Edge values") {
            uint8_t src[] = {0, 128, 255};
            float dst[3];
            
            converter(dst, reinterpret_cast<const uint8_t*>(src), 3);
            
            CHECK(dst[0] <= -0.99f);
            CHECK(std::abs(dst[1]) < 0.01f);
            CHECK(dst[2] >= 0.99f);
        }
    }
    
    TEST_CASE("S16 to float conversion") {
        auto converter = musac::get_to_float_conveter(musac::audio_s16sys);
        REQUIRE(converter != nullptr);
        
        SUBCASE("Basic conversion") {
            int16_t src[] = {-32768, -16384, 0, 16384, 32767};
            float dst[5];
            
            converter(dst, reinterpret_cast<const uint8_t*>(src), 5);
            
            CHECK(dst[0] == doctest::Approx(-1.0f));
            CHECK(dst[1] == doctest::Approx(-0.5f));
            CHECK(dst[2] == doctest::Approx(0.0f));
            CHECK(dst[3] == doctest::Approx(0.5f));
            CHECK(dst[4] == doctest::Approx(1.0f).epsilon(0.0001f));
        }
        
        SUBCASE("Small values") {
            int16_t src[] = {-1, 0, 1};
            float dst[3];
            
            converter(dst, reinterpret_cast<const uint8_t*>(src), 3);
            
            CHECK(dst[0] < 0.0f);
            CHECK(dst[1] == 0.0f);
            CHECK(dst[2] > 0.0f);
            CHECK(std::abs(dst[0]) < 0.001f);
            CHECK(std::abs(dst[2]) < 0.001f);
        }
    }
    
    TEST_CASE("S32 to float conversion") {
        auto converter = musac::get_to_float_conveter(musac::audio_s32sys);
        REQUIRE(converter != nullptr);
        
        SUBCASE("Basic conversion") {
            int32_t src[] = {INT32_MIN, INT32_MIN/2, 0, INT32_MAX/2, INT32_MAX};
            float dst[5];
            
            converter(dst, reinterpret_cast<const uint8_t*>(src), 5);
            
            CHECK(dst[0] == doctest::Approx(-1.0f));
            CHECK(dst[1] == doctest::Approx(-0.5f));
            CHECK(dst[2] == doctest::Approx(0.0f));
            CHECK(dst[3] == doctest::Approx(0.5f));
            CHECK(dst[4] == doctest::Approx(1.0f).epsilon(0.0001f));
        }
    }
    
    TEST_CASE("F32 to float conversion") {
        auto converter = musac::get_to_float_conveter(musac::audio_f32sys);
        REQUIRE(converter != nullptr);
        
        SUBCASE("Pass-through") {
            float src[] = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
            float dst[5];
            
            converter(dst, reinterpret_cast<const uint8_t*>(src), 5);
            
            // Should be a direct copy
            CHECK(dst[0] == src[0]);
            CHECK(dst[1] == src[1]);
            CHECK(dst[2] == src[2]);
            CHECK(dst[3] == src[3]);
            CHECK(dst[4] == src[4]);
        }
        
        SUBCASE("Out of range values") {
            float src[] = {-2.0f, 2.0f, -1.5f, 1.5f};
            float dst[4];
            
            converter(dst, reinterpret_cast<const uint8_t*>(src), 4);
            
            // Converter might clip or pass through
            // This depends on implementation
            CHECK(std::abs(dst[0]) >= 1.0f);
            CHECK(std::abs(dst[1]) >= 1.0f);
        }
    }
    
    TEST_CASE("Large buffer conversion") {
        auto converter = musac::get_to_float_conveter(musac::audio_s16sys);
        REQUIRE(converter != nullptr);
        
        // Create a sine wave
        const size_t sample_count = 4096;
        std::vector<int16_t> src(sample_count);
        std::vector<float> dst(sample_count);
        
        for (size_t i = 0; i < sample_count; i++) {
            float phase = static_cast<float>(i) / static_cast<float>(sample_count) * 2.0f * static_cast<float>(M_PI);
            src[i] = static_cast<int16_t>(std::sin(phase) * 32767.0f);
        }
        
        converter(dst.data(), reinterpret_cast<const uint8_t*>(src.data()), sample_count);
        
        // Verify the sine wave is preserved
        for (size_t i = 0; i < sample_count; i++) {
            float expected = static_cast<float>(src[i]) / 32768.0f;
            CHECK(dst[i] == doctest::Approx(expected).epsilon(0.0001f));
        }
        
        // Check peak values
        auto [min_it, max_it] = std::minmax_element(dst.begin(), dst.end());
        CHECK(*min_it >= -1.0f);
        CHECK(*max_it <= 1.0f);
    }
    
    TEST_CASE("Null and edge cases") {
        SUBCASE("Unknown format") {
            auto converter = musac::get_to_float_conveter(musac::audio_format::unknown);
            CHECK(converter == nullptr);
        }
        
        SUBCASE("Zero samples") {
            auto converter = musac::get_to_float_conveter(musac::audio_s16sys);
            REQUIRE(converter != nullptr);
            
            int16_t src[1] = {1000};
            float dst[1] = {999.0f};
            
            converter(dst, reinterpret_cast<const uint8_t*>(src), 0);
            
            // Should not modify dst
            CHECK(dst[0] == 999.0f);
        }
    }
}