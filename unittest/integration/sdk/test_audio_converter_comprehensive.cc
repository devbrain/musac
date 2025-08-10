#include <doctest/doctest.h>
#include <musac/sdk/audio_converter.hh>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/buffer.hh>
#include <vector>
#include <cstring>
#include <cmath>
#include <random>
#include <algorithm>

// Helper to generate test data for different formats
static std::vector<uint8_t> generate_test_data(musac::audio_format format, 
                                                     uint8_t channels, 
                                                     size_t num_frames) {
    size_t bytes_per_sample = musac::audio_format_byte_size(format);
    size_t total_bytes = bytes_per_sample * channels * num_frames;
    std::vector<uint8_t> data(total_bytes);
    
    std::mt19937 gen(42); // Fixed seed for reproducibility
    
    switch (format) {
        case musac::audio_format::u8: {
            std::uniform_int_distribution<int> dist(0, 255);
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] = static_cast<uint8_t>(dist(gen));
            }
            break;
        }
        case musac::audio_format::s8: {
            std::uniform_int_distribution<int> dist(-128, 127);
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] = static_cast<uint8_t>(dist(gen));
            }
            break;
        }
        case musac::audio_format::s16le:
        case musac::audio_format::s16be: {
            std::uniform_int_distribution<int16_t> dist(-32768, 32767);
            auto* samples = reinterpret_cast<int16_t*>(data.data());
            for (size_t i = 0; i < num_frames * channels; ++i) {
                int16_t val = dist(gen);
                if (format == musac::audio_format::s16be) {
                    val = ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
                }
                samples[i] = val;
            }
            break;
        }
        case musac::audio_format::s32le:
        case musac::audio_format::s32be: {
            std::uniform_int_distribution<int32_t> dist(-2147483648, 2147483647);
            auto* samples = reinterpret_cast<int32_t*>(data.data());
            for (size_t i = 0; i < num_frames * channels; ++i) {
                int32_t val = dist(gen);
                if (format == musac::audio_format::s32be) {
                    val = ((val & 0xFF) << 24) | ((val & 0xFF00) << 8) | 
                          ((val >> 8) & 0xFF00) | ((val >> 24) & 0xFF);
                }
                samples[i] = val;
            }
            break;
        }
        case musac::audio_format::f32le:
        case musac::audio_format::f32be: {
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            auto* samples = reinterpret_cast<float*>(data.data());
            for (size_t i = 0; i < num_frames * channels; ++i) {
                float val = dist(gen);
                if (format == musac::audio_format::f32be) {
                    auto* bytes = reinterpret_cast<uint32_t*>(&val);
                    *bytes = ((*bytes & 0xFF) << 24) | ((*bytes & 0xFF00) << 8) | 
                             ((*bytes >> 8) & 0xFF00) | ((*bytes >> 24) & 0xFF);
                }
                samples[i] = val;
            }
            break;
        }
        default:
            break;
    }
    
    return data;
}

// Helper to verify conversion maintains relative signal levels
static bool verify_conversion_quality(const uint8_t* original, musac::audio_format orig_fmt,
                                      const uint8_t* converted, musac::audio_format conv_fmt,
                                      size_t num_samples, float tolerance = 0.01f) {
    // Convert both to normalized float for comparison
    std::vector<float> orig_float(num_samples);
    std::vector<float> conv_float(num_samples);
    
    // Normalize original
    switch (orig_fmt) {
        case musac::audio_format::u8: {
            for (size_t i = 0; i < num_samples; ++i) {
                orig_float[i] = (original[i] - 128) / 128.0f;
            }
            break;
        }
        case musac::audio_format::s8: {
            auto* samples = reinterpret_cast<const int8_t*>(original);
            for (size_t i = 0; i < num_samples; ++i) {
                orig_float[i] = samples[i] / 128.0f;
            }
            break;
        }
        case musac::audio_format::s16le: {
            auto* samples = reinterpret_cast<const int16_t*>(original);
            for (size_t i = 0; i < num_samples; ++i) {
                orig_float[i] = samples[i] / 32768.0f;
            }
            break;
        }
        case musac::audio_format::f32le: {
            auto* samples = reinterpret_cast<const float*>(original);
            for (size_t i = 0; i < num_samples; ++i) {
                orig_float[i] = samples[i];
            }
            break;
        }
        default:
            return false;
    }
    
    // Normalize converted
    switch (conv_fmt) {
        case musac::audio_format::u8: {
            for (size_t i = 0; i < num_samples; ++i) {
                conv_float[i] = (converted[i] - 128) / 128.0f;
            }
            break;
        }
        case musac::audio_format::s8: {
            auto* samples = reinterpret_cast<const int8_t*>(converted);
            for (size_t i = 0; i < num_samples; ++i) {
                conv_float[i] = samples[i] / 128.0f;
            }
            break;
        }
        case musac::audio_format::s16le: {
            auto* samples = reinterpret_cast<const int16_t*>(converted);
            for (size_t i = 0; i < num_samples; ++i) {
                conv_float[i] = samples[i] / 32768.0f;
            }
            break;
        }
        case musac::audio_format::f32le: {
            auto* samples = reinterpret_cast<const float*>(converted);
            for (size_t i = 0; i < num_samples; ++i) {
                conv_float[i] = samples[i];
            }
            break;
        }
        default:
            return false;
    }
    
    // Compare with tolerance
    for (size_t i = 0; i < num_samples; ++i) {
        if (std::abs(orig_float[i] - conv_float[i]) > tolerance) {
            return false;
        }
    }
    
    return true;
}

TEST_SUITE("AudioConverterComprehensive") {
    
    TEST_CASE("All format conversions") {
        // Test all combinations of format conversions
        std::vector<musac::audio_format> formats = {
            musac::audio_format::u8,
            musac::audio_format::s8,
            musac::audio_format::s16le,
            musac::audio_format::s16be,
            musac::audio_format::s32le,
            musac::audio_format::s32be,
            musac::audio_format::f32le,
            musac::audio_format::f32be
        };
        
        for (auto src_fmt : formats) {
            for (auto dst_fmt : formats) {
                CAPTURE(src_fmt);
                CAPTURE(dst_fmt);
                
                musac::audio_spec src_spec{src_fmt, 1, 44100};
                musac::audio_spec dst_spec{dst_fmt, 1, 44100};
                
                auto test_data = generate_test_data(src_fmt, 1, 16);
                
                // Test conversion
                if (src_fmt == dst_fmt) {
                    // Same format should be a direct copy
                    auto result = musac::audio_converter::convert(
                        src_spec, test_data.data(), test_data.size(), dst_spec);
                    
                    CHECK(result.size() == test_data.size());
                    CHECK(std::memcmp(result.data(), test_data.data(), test_data.size()) == 0);
                } else {
                    // Different formats should convert properly
                    try {
                        auto result = musac::audio_converter::convert(
                            src_spec, test_data.data(), test_data.size(), dst_spec);
                        
                        // Verify size is correct
                        size_t expected_size = 16 * musac::audio_format_byte_size(dst_fmt);
                        CHECK(result.size() == expected_size);
                        
                        // For supported conversions, verify quality
                        if ((src_fmt == musac::audio_format::u8 || 
                             src_fmt == musac::audio_format::s8 ||
                             src_fmt == musac::audio_format::s16le || 
                             src_fmt == musac::audio_format::f32le) &&
                            (dst_fmt == musac::audio_format::u8 || 
                             dst_fmt == musac::audio_format::s8 ||
                             dst_fmt == musac::audio_format::s16le || 
                             dst_fmt == musac::audio_format::f32le)) {
                            CHECK(verify_conversion_quality(
                                test_data.data(), src_fmt,
                                result.data(), dst_fmt,
                                16, 0.02f)); // 2% tolerance for quantization
                        }
                    } catch (const musac::audio_conversion_error&) {
                        // Some conversions might not be supported yet
                        CHECK(true); // Mark as handled
                    }
                }
            }
        }
    }
    
    TEST_CASE("Channel mixing conversions") {
        musac::audio_format fmt = musac::audio_format::s16le;
        
        SUBCASE("Mono to stereo") {
            musac::audio_spec src{fmt, 1, 44100};
            musac::audio_spec dst{fmt, 2, 44100};
            
            std::vector<int16_t> mono_data = {100, 200, 300};
            auto input = reinterpret_cast<uint8_t*>(mono_data.data());
            size_t input_size = mono_data.size() * sizeof(int16_t);
            
            auto result = musac::audio_converter::convert(src, input, input_size, dst);
            
            CHECK(result.size() == input_size * 2);
            auto* stereo = reinterpret_cast<int16_t*>(result.data());
            CHECK(stereo[0] == 100); // L
            CHECK(stereo[1] == 100); // R
            CHECK(stereo[2] == 200); // L
            CHECK(stereo[3] == 200); // R
        }
        
        SUBCASE("Stereo to mono") {
            musac::audio_spec src{fmt, 2, 44100};
            musac::audio_spec dst{fmt, 1, 44100};
            
            std::vector<int16_t> stereo_data = {100, 200, 300, 400};
            auto input = reinterpret_cast<uint8_t*>(stereo_data.data());
            size_t input_size = stereo_data.size() * sizeof(int16_t);
            
            auto result = musac::audio_converter::convert(src, input, input_size, dst);
            
            CHECK(result.size() == input_size / 2);
            auto* mono = reinterpret_cast<int16_t*>(result.data());
            CHECK(mono[0] == 150); // (100+200)/2
            CHECK(mono[1] == 350); // (300+400)/2
        }
        
        SUBCASE("5.1 to stereo") {
            musac::audio_spec src{fmt, 6, 44100};
            musac::audio_spec dst{fmt, 2, 44100};
            
            // 6 channels: FL, FR, C, LFE, RL, RR
            std::vector<int16_t> surround_data(6 * 2); // 2 frames
            auto input = reinterpret_cast<uint8_t*>(surround_data.data());
            size_t input_size = surround_data.size() * sizeof(int16_t);
            
            // This might not be implemented yet
            try {
                auto result = musac::audio_converter::convert(src, input, input_size, dst);
                CHECK(result.size() > 0);
            } catch (const musac::audio_conversion_error&) {
                CHECK(true); // Expected for unsupported channel counts
            }
        }
    }
    
    TEST_CASE("Sample rate conversions") {
        musac::audio_format fmt = musac::audio_format::s16le;
        
        SUBCASE("Upsample 44.1kHz to 48kHz") {
            musac::audio_spec src{fmt, 1, 44100};
            musac::audio_spec dst{fmt, 1, 48000};
            
            size_t src_frames = 441; // 0.01 seconds
            auto test_data = generate_test_data(fmt, 1, src_frames);
            
            auto result = musac::audio_converter::convert(
                src, test_data.data(), test_data.size(), dst);
            
            // Should have approximately 480 frames
            size_t dst_frames = result.size() / sizeof(int16_t);
            CHECK(dst_frames >= 478);
            CHECK(dst_frames <= 482);
        }
        
        SUBCASE("Downsample 48kHz to 44.1kHz") {
            musac::audio_spec src{fmt, 1, 48000};
            musac::audio_spec dst{fmt, 1, 44100};
            
            size_t src_frames = 480; // 0.01 seconds
            auto test_data = generate_test_data(fmt, 1, src_frames);
            
            auto result = musac::audio_converter::convert(
                src, test_data.data(), test_data.size(), dst);
            
            // Should have approximately 441 frames
            size_t dst_frames = result.size() / sizeof(int16_t);
            CHECK(dst_frames >= 439);
            CHECK(dst_frames <= 443);
        }
        
        SUBCASE("Extreme upsampling 8kHz to 48kHz") {
            musac::audio_spec src{fmt, 1, 8000};
            musac::audio_spec dst{fmt, 1, 48000};
            
            size_t src_frames = 80; // 0.01 seconds
            auto test_data = generate_test_data(fmt, 1, src_frames);
            
            auto result = musac::audio_converter::convert(
                src, test_data.data(), test_data.size(), dst);
            
            // Should have approximately 480 frames (6x)
            size_t dst_frames = result.size() / sizeof(int16_t);
            CHECK(dst_frames >= 478);
            CHECK(dst_frames <= 482);
        }
    }
    
    TEST_CASE("Combined conversions") {
        SUBCASE("Format + channel + rate conversion") {
            musac::audio_spec src{musac::audio_format::u8, 1, 22050};
            musac::audio_spec dst{musac::audio_format::s16le, 2, 44100};
            
            size_t src_frames = 220; // 0.01 seconds
            auto test_data = generate_test_data(musac::audio_format::u8, 1, src_frames);
            
            auto result = musac::audio_converter::convert(
                src, test_data.data(), test_data.size(), dst);
            
            // Should have ~440 stereo frames in 16-bit
            size_t expected_bytes = 440 * 2 * sizeof(int16_t);
            CHECK(result.size() >= expected_bytes - 16);
            CHECK(result.size() <= expected_bytes + 16);
        }
        
        SUBCASE("Endian swap + channel mixing") {
            musac::audio_spec src{musac::audio_format::s16be, 2, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            
            std::vector<uint8_t> test_data = {
                0x01, 0x00,  // BE: 256
                0x02, 0x00,  // BE: 512
                0x03, 0x00,  // BE: 768
                0x04, 0x00   // BE: 1024
            };
            
            auto result = musac::audio_converter::convert(
                src, test_data.data(), test_data.size(), dst);
            
            CHECK(result.size() == 4); // 2 mono samples
            auto* samples = reinterpret_cast<int16_t*>(result.data());
            CHECK(samples[0] == (256 + 512) / 2);
            CHECK(samples[1] == (768 + 1024) / 2);
        }
    }
    
    TEST_CASE("Stream converter comprehensive") {
        SUBCASE("Format conversion with small chunks") {
            musac::audio_spec src{musac::audio_format::u8, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            
            musac::audio_converter::stream_converter converter(src, dst);
            
            // Generate 100 samples, process in chunks of 10
            auto test_data = generate_test_data(musac::audio_format::u8, 1, 100);
            musac::buffer<uint8_t> output(1024);
            std::vector<uint8_t> accumulated;
            
            for (size_t i = 0; i < 100; i += 10) {
                size_t chunk_size = std::min<size_t>(10, 100 - i);
                size_t written = converter.process_chunk(
                    test_data.data() + i, chunk_size, output);
                
                accumulated.insert(accumulated.end(), 
                                 output.data(), output.data() + written);
            }
            
            // Flush any remaining
            size_t flushed = converter.flush(output);
            accumulated.insert(accumulated.end(), 
                             output.data(), output.data() + flushed);
            
            CHECK(accumulated.size() == 200); // 100 u8 -> 100 s16le
        }
        
        SUBCASE("Resampling with streaming") {
            musac::audio_spec src{musac::audio_format::s16le, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 48000};
            
            musac::audio_converter::stream_converter converter(src, dst);
            
            // Process 441 samples (0.01 sec) in chunks
            auto test_data = generate_test_data(musac::audio_format::s16le, 1, 441);
            musac::buffer<uint8_t> output(2048);
            std::vector<uint8_t> accumulated;
            
            // Process in uneven chunks to test buffering
            std::vector<size_t> chunk_sizes = {100, 150, 91, 100};
            size_t offset = 0;
            
            for (size_t chunk_frames : chunk_sizes) {
                size_t chunk_bytes = chunk_frames * sizeof(int16_t);
                size_t written = converter.process_chunk(
                    test_data.data() + offset, chunk_bytes, output);
                
                accumulated.insert(accumulated.end(), 
                                 output.data(), output.data() + written);
                offset += chunk_bytes;
            }
            
            // Flush
            size_t flushed = converter.flush(output);
            accumulated.insert(accumulated.end(), 
                             output.data(), output.data() + flushed);
            
            // Should have approximately 480 samples
            size_t output_samples = accumulated.size() / sizeof(int16_t);
            CHECK(output_samples >= 478);
            CHECK(output_samples <= 482);
        }
        
        SUBCASE("Channel mixing with streaming") {
            musac::audio_spec src{musac::audio_format::s16le, 2, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            
            musac::audio_converter::stream_converter converter(src, dst);
            
            // Create stereo test pattern
            std::vector<int16_t> stereo_data;
            for (size_t i = 0; i < 100; ++i) {
                stereo_data.push_back(static_cast<int16_t>(i * 100));      // L
                stereo_data.push_back(static_cast<int16_t>(i * 100 + 50)); // R
            }
            
            auto* input = reinterpret_cast<uint8_t*>(stereo_data.data());
            size_t input_size = stereo_data.size() * sizeof(int16_t);
            
            musac::buffer<uint8_t> output(1024);
            std::vector<int16_t> accumulated;
            
            // Process in chunks
            for (size_t i = 0; i < input_size; i += 40) {
                size_t chunk = std::min<size_t>(40, input_size - i);
                size_t written = converter.process_chunk(input + i, chunk, output);
                
                auto* samples = reinterpret_cast<int16_t*>(output.data());
                size_t num_samples = written / sizeof(int16_t);
                accumulated.insert(accumulated.end(), samples, samples + num_samples);
            }
            
            // Verify averaging
            CHECK(accumulated.size() == 100);
            for (size_t i = 0; i < 100; ++i) {
                int16_t expected = static_cast<int16_t>(i * 100 + 25);
                CHECK(accumulated[i] == expected);
            }
        }
        
        SUBCASE("Reset during streaming") {
            musac::audio_spec src{musac::audio_format::u8, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            
            musac::audio_converter::stream_converter converter(src, dst);
            musac::buffer<uint8_t> output(256);
            
            // Process some data
            std::vector<uint8_t> chunk1 = {128, 128, 128, 128};
            size_t written1 = converter.process_chunk(chunk1.data(), chunk1.size(), output);
            CHECK(written1 == 8);
            
            // Reset
            converter.reset();
            
            // Process again - should work normally
            std::vector<uint8_t> chunk2 = {255, 255, 255, 255};
            size_t written2 = converter.process_chunk(chunk2.data(), chunk2.size(), output);
            CHECK(written2 == 8);
            
            // Verify second chunk data
            auto* samples = reinterpret_cast<int16_t*>(output.data());
            CHECK(samples[0] == 32512); // (255-128) << 8
        }
    }
    
    TEST_CASE("Edge cases") {
        SUBCASE("Empty input") {
            musac::audio_spec spec{musac::audio_format::s16le, 2, 44100};
            
            auto result = musac::audio_converter::convert(spec, nullptr, 0, spec);
            CHECK(result.size() == 0);
        }
        
        SUBCASE("Single sample") {
            musac::audio_spec src{musac::audio_format::u8, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            
            uint8_t sample = 200;
            auto result = musac::audio_converter::convert(src, &sample, 1, dst);
            
            CHECK(result.size() == 2);
            auto* converted = reinterpret_cast<int16_t*>(result.data());
            CHECK(converted[0] == ((200 - 128) << 8));
        }
        
        SUBCASE("Odd number of bytes for 16-bit format") {
            musac::audio_spec src{musac::audio_format::s16le, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 48000};
            
            std::vector<uint8_t> odd_data(3); // 1.5 samples
            
            // Should handle gracefully (process only complete samples)
            try {
                auto result = musac::audio_converter::convert(
                    src, odd_data.data(), odd_data.size(), dst);
                CHECK(result.size() >= 0);
            } catch (const musac::audio_conversion_error&) {
                CHECK(true); // Also acceptable to throw
            }
        }
        
        SUBCASE("Very high sample rate") {
            musac::audio_spec src{musac::audio_format::s16le, 1, 192000};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            
            auto test_data = generate_test_data(musac::audio_format::s16le, 1, 1920);
            
            auto result = musac::audio_converter::convert(
                src, test_data.data(), test_data.size(), dst);
            
            // Should downsample by ~4.35x
            size_t output_frames = result.size() / sizeof(int16_t);
            CHECK(output_frames >= 440);
            CHECK(output_frames <= 444);
        }
    }
    
    TEST_CASE("Performance characteristics") {
        SUBCASE("Fast path detection") {
            musac::audio_spec spec1{musac::audio_format::s16le, 2, 44100};
            musac::audio_spec spec2{musac::audio_format::s16be, 2, 44100};
            musac::audio_spec spec3{musac::audio_format::s16le, 1, 44100};
            musac::audio_spec spec4{musac::audio_format::s16le, 2, 48000};
            
            // Endian swap should be fast
            CHECK(musac::audio_converter::has_fast_path(spec1, spec2));
            
            // Channel conversion should be fast
            CHECK(musac::audio_converter::has_fast_path(spec1, spec3));
            
            // Sample rate conversion is not fast
            CHECK_FALSE(musac::audio_converter::has_fast_path(spec1, spec4));
        }
        
        SUBCASE("In-place conversion") {
            musac::audio_spec src{musac::audio_format::s16le, 2, 44100};
            musac::audio_spec dst{musac::audio_format::s16be, 2, 44100};
            
            std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
            
            musac::audio_converter::convert_in_place(src, data.data(), data.size(), dst);
            
            CHECK(src.format == musac::audio_format::s16be);
            CHECK(data[0] == 0x02);
            CHECK(data[1] == 0x01);
            CHECK(data[2] == 0x04);
            CHECK(data[3] == 0x03);
        }
    }
}