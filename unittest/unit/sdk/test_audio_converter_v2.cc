#include <doctest/doctest.h>
#include <musac/sdk/audio_converter.hh>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/buffer.hh>
#include <vector>
#include <cstring>

TEST_SUITE("AudioConverterV2") {
    TEST_CASE("needs_conversion detects when conversion is needed") {
        musac::audio_spec spec1{musac::audio_format::s16le, 2, 44100};
        musac::audio_spec spec2{musac::audio_format::s16le, 2, 44100};
        musac::audio_spec spec3{musac::audio_format::s16be, 2, 44100};
        musac::audio_spec spec4{musac::audio_format::s16le, 1, 44100};
        musac::audio_spec spec5{musac::audio_format::s16le, 2, 48000};
        
        SUBCASE("identical specs need no conversion") {
            CHECK_FALSE(musac::audio_converter::needs_conversion(spec1, spec2));
        }
        
        SUBCASE("different format needs conversion") {
            CHECK(musac::audio_converter::needs_conversion(spec1, spec3));
        }
        
        SUBCASE("different channels needs conversion") {
            CHECK(musac::audio_converter::needs_conversion(spec1, spec4));
        }
        
        SUBCASE("different sample rate needs conversion") {
            CHECK(musac::audio_converter::needs_conversion(spec1, spec5));
        }
    }
    
    TEST_CASE("has_fast_path detects optimized conversions") {
        musac::audio_spec mono_44k{musac::audio_format::s16le, 1, 44100};
        musac::audio_spec stereo_44k{musac::audio_format::s16le, 2, 44100};
        musac::audio_spec mono_48k{musac::audio_format::s16le, 1, 48000};
        musac::audio_spec s16le{musac::audio_format::s16le, 2, 44100};
        musac::audio_spec s16be{musac::audio_format::s16be, 2, 44100};
        
        SUBCASE("endian swap is fast path") {
            CHECK(musac::audio_converter::has_fast_path(s16le, s16be));
            CHECK(musac::audio_converter::has_fast_path(s16be, s16le));
        }
        
        SUBCASE("mono to stereo is fast path") {
            CHECK(musac::audio_converter::has_fast_path(mono_44k, stereo_44k));
        }
        
        SUBCASE("stereo to mono is fast path") {
            CHECK(musac::audio_converter::has_fast_path(stereo_44k, mono_44k));
        }
        
        SUBCASE("sample rate conversion is not fast path") {
            CHECK_FALSE(musac::audio_converter::has_fast_path(mono_44k, mono_48k));
        }
    }
    
    TEST_CASE("estimate_output_size calculates correct sizes") {
        SUBCASE("same format and rate") {
            musac::audio_spec src{musac::audio_format::s16le, 2, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 2, 44100};
            size_t input_size = 1000;
            
            size_t estimated = musac::audio_converter::estimate_output_size(src, input_size, dst);
            CHECK(estimated == input_size);
        }
        
        SUBCASE("mono to stereo doubles size") {
            musac::audio_spec src{musac::audio_format::s16le, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 2, 44100};
            size_t input_size = 1000;
            
            size_t estimated = musac::audio_converter::estimate_output_size(src, input_size, dst);
            CHECK(estimated == input_size * 2);
        }
        
        SUBCASE("stereo to mono halves size") {
            musac::audio_spec src{musac::audio_format::s16le, 2, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            size_t input_size = 1000;
            
            size_t estimated = musac::audio_converter::estimate_output_size(src, input_size, dst);
            CHECK(estimated == input_size / 2);
        }
        
        SUBCASE("8-bit to 16-bit doubles size") {
            musac::audio_spec src{musac::audio_format::u8, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            size_t input_size = 1000;
            
            size_t estimated = musac::audio_converter::estimate_output_size(src, input_size, dst);
            CHECK(estimated == input_size * 2);
        }
        
        SUBCASE("sample rate conversion adjusts size") {
            musac::audio_spec src{musac::audio_format::s16le, 2, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 2, 48000};
            size_t input_size = 44100 * 2 * 2; // 1 second of stereo 16-bit
            
            size_t estimated = musac::audio_converter::estimate_output_size(src, input_size, dst);
            // Should be approximately 48000 * 2 * 2 + small buffer
            CHECK(estimated >= 48000 * 2 * 2);
            CHECK(estimated <= 48000 * 2 * 2 + 32); // 4 frames buffer * 2 channels * 2 bytes
        }
    }
    
    TEST_CASE("convert basic functionality") {
        SUBCASE("convert u8 to s16le") {
            musac::audio_spec src{musac::audio_format::u8, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            
            std::vector<musac::uint8> src_data = {0, 64, 128, 192, 255};
            
            musac::buffer<musac::uint8> result = musac::audio_converter::convert(
                src, src_data.data(), src_data.size(), dst);
            
            CHECK(result.size() == src_data.size() * 2);
            
            // Check converted values
            musac::int16* samples = reinterpret_cast<musac::int16*>(result.data());
            CHECK(samples[0] == -32768); // 0 -> -128 -> -32768
            CHECK(samples[2] == 0);      // 128 -> 0 -> 0  
            CHECK(samples[4] == 32512);  // 255 -> 127 -> 32512
        }
        
        SUBCASE("convert mono to stereo") {
            musac::audio_spec src{musac::audio_format::s16le, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 2, 44100};
            
            std::vector<musac::int16> src_samples = {100, 200, 300};
            std::vector<musac::uint8> src_data(src_samples.size() * sizeof(musac::int16));
            std::memcpy(src_data.data(), src_samples.data(), src_data.size());
            
            musac::buffer<musac::uint8> result = musac::audio_converter::convert(
                src, src_data.data(), src_data.size(), dst);
            
            CHECK(result.size() == src_data.size() * 2);
            
            musac::int16* samples = reinterpret_cast<musac::int16*>(result.data());
            CHECK(samples[0] == 100); // Left
            CHECK(samples[1] == 100); // Right
            CHECK(samples[2] == 200); // Left
            CHECK(samples[3] == 200); // Right
        }
        
        SUBCASE("empty input produces empty output") {
            musac::audio_spec src{musac::audio_format::s16le, 2, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 2, 48000};
            
            musac::buffer<musac::uint8> result = musac::audio_converter::convert(
                src, nullptr, 0, dst);
            
            CHECK(result.size() == 0);
        }
    }
    
    TEST_CASE("convert_in_place for endian swapping") {
        SUBCASE("swap s16le to s16be in place") {
            musac::audio_spec spec{musac::audio_format::s16le, 2, 44100};
            musac::audio_spec dst_spec{musac::audio_format::s16be, 2, 44100};
            
            std::vector<musac::int16> samples = {0x0102, 0x0304, 0x0506, 0x0708};
            std::vector<musac::uint8> data(samples.size() * sizeof(musac::int16));
            std::memcpy(data.data(), samples.data(), data.size());
            
            musac::audio_converter::convert_in_place(spec, data.data(), data.size(), dst_spec);
            
            CHECK(spec.format == musac::audio_format::s16be);
            
            musac::int16* converted = reinterpret_cast<musac::int16*>(data.data());
            CHECK(converted[0] == 0x0201);
            CHECK(converted[1] == 0x0403);
            CHECK(converted[2] == 0x0605);
            CHECK(converted[3] == 0x0807);
        }
        
        SUBCASE("throws when in-place not possible") {
            musac::audio_spec spec{musac::audio_format::s16le, 1, 44100};
            musac::audio_spec dst_spec{musac::audio_format::s16le, 2, 44100};
            
            std::vector<musac::uint8> data(100);
            
            CHECK_THROWS_AS(
                musac::audio_converter::convert_in_place(spec, data.data(), data.size(), dst_spec),
                musac::audio_conversion_error);
        }
    }
    
    TEST_CASE("convert_into with pre-allocated buffer") {
        SUBCASE("converts into existing buffer") {
            musac::audio_spec src{musac::audio_format::u8, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            
            std::vector<musac::uint8> src_data = {0, 64, 128, 192, 255};
            musac::buffer<musac::uint8> dst_buffer(100); // Pre-allocate larger buffer
            
            size_t written = musac::audio_converter::convert_into(
                src, src_data.data(), src_data.size(), dst, dst_buffer);
            
            CHECK(written == src_data.size() * 2);
            CHECK(dst_buffer.size() >= written);
            
            musac::int16* samples = reinterpret_cast<musac::int16*>(dst_buffer.data());
            CHECK(samples[0] == -32768);
        }
        
        SUBCASE("resizes buffer if too small") {
            musac::audio_spec src{musac::audio_format::s16le, 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 2, 44100};
            
            std::vector<musac::uint8> src_data(100);
            musac::buffer<musac::uint8> dst_buffer(10); // Too small
            
            size_t written = musac::audio_converter::convert_into(
                src, src_data.data(), src_data.size(), dst, dst_buffer);
            
            CHECK(written == src_data.size() * 2);
            CHECK(dst_buffer.size() >= written);
        }
    }
    
    TEST_CASE("stream_converter for chunked processing") {
        musac::audio_spec src{musac::audio_format::u8, 1, 44100};
        musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
        
        musac::audio_converter::stream_converter converter(src, dst);
        
        SUBCASE("process single chunk") {
            std::vector<musac::uint8> input = {128, 128, 128, 128};
            musac::buffer<musac::uint8> output(100);
            
            size_t written = converter.process_chunk(input.data(), input.size(), output);
            
            CHECK(written == input.size() * 2);
            
            musac::int16* samples = reinterpret_cast<musac::int16*>(output.data());
            CHECK(samples[0] == 0);
            CHECK(samples[1] == 0);
            CHECK(samples[2] == 0);
            CHECK(samples[3] == 0);
        }
        
        SUBCASE("process multiple small chunks") {
            // Send data in small chunks
            std::vector<musac::uint8> chunk1 = {0, 64};
            std::vector<musac::uint8> chunk2 = {128, 192};
            std::vector<musac::uint8> chunk3 = {255};
            musac::buffer<musac::uint8> output(100);
            
            // Process chunks
            size_t written1 = converter.process_chunk(chunk1.data(), chunk1.size(), output);
            size_t written2 = converter.process_chunk(chunk2.data(), chunk2.size(), output);
            size_t written3 = converter.process_chunk(chunk3.data(), chunk3.size(), output);
            
            // Should get output when enough data accumulated
            size_t total = written1 + written2 + written3;
            CHECK(total > 0);
            
            // Flush remaining
            size_t flushed = converter.flush(output);
            CHECK((total + flushed) == 10); // 5 input bytes -> 10 output bytes
        }
        
        SUBCASE("flush returns remaining data") {
            std::vector<musac::uint8> input = {128, 128, 128};
            musac::buffer<musac::uint8> output(4);  // Small output buffer
            
            size_t written = converter.process_chunk(input.data(), input.size(), output);
            CHECK(written == 4); // Only fits 2 samples
            
            // Flush should return the remaining sample
            size_t flushed = converter.flush(output);
            CHECK(flushed == 2);
        }
        
        SUBCASE("reset clears internal state") {
            std::vector<musac::uint8> input = {128, 128};
            musac::buffer<musac::uint8> output(100);
            
            converter.process_chunk(input.data(), input.size(), output);
            converter.reset();
            
            // After reset, should work normally
            size_t written = converter.process_chunk(input.data(), input.size(), output);
            CHECK(written == input.size() * 2);
        }
        
        SUBCASE("handles format conversion with chunking") {
            musac::audio_spec src_stereo{musac::audio_format::s16le, 2, 44100};
            musac::audio_spec dst_mono{musac::audio_format::s16le, 1, 44100};
            musac::audio_converter::stream_converter stereo_to_mono(src_stereo, dst_mono);
            
            // 4 bytes = 1 stereo frame (2 * int16)
            std::vector<musac::uint8> stereo_data = {
                0x00, 0x01,  // Left: 256
                0x00, 0x02,  // Right: 512
                0x00, 0x03,  // Left: 768
                0x00, 0x04   // Right: 1024
            };
            
            musac::buffer<musac::uint8> mono_output(100);
            size_t written = stereo_to_mono.process_chunk(stereo_data.data(), stereo_data.size(), mono_output);
            
            CHECK(written == 4); // 2 stereo frames -> 2 mono frames
            musac::int16* samples = reinterpret_cast<musac::int16*>(mono_output.data());
            CHECK(samples[0] == (256 + 512) / 2);  // Average of first frame
            CHECK(samples[1] == (768 + 1024) / 2); // Average of second frame
        }
    }
    
    TEST_CASE("error handling") {
        SUBCASE("throws on unsupported format") {
            musac::audio_spec src{static_cast<musac::audio_format>(0xFFFF), 1, 44100};
            musac::audio_spec dst{musac::audio_format::s16le, 1, 44100};
            
            CHECK_THROWS_AS(
                musac::audio_converter::estimate_output_size(src, 100, dst),
                musac::unsupported_format_error);
        }
    }
}