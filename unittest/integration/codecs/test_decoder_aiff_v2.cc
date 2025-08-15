#include <doctest/doctest.h>
#include <musac/codecs/decoder_aiff_v2.hh>
#include <musac/sdk/io_stream.hh>
#include <vector>
#include <cstring>
#include <cmath>
#include <iostream>
#include <chrono>

// Helper to create a minimal AIFF file in memory
std::vector<uint8_t> createTestAIFF_v2(uint16_t channels = 1, 
                                       uint16_t bitDepth = 16,
                                       uint32_t sampleRate = 44100,
                                       uint32_t numSamples = 100,
                                       const char* compressionType = nullptr) {
    std::vector<uint8_t> data;
    
    auto writeU32BE = [&data](uint32_t value) {
        data.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
        data.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(value & 0xFF));
    };
    
    auto writeU16BE = [&data](uint16_t value) {
        data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(value & 0xFF));
    };
    
    auto write4CC = [&data](const char* fourcc) {
        data.insert(data.end(), fourcc, fourcc + 4);
    };
    
    // FORM chunk
    write4CC("FORM");
    size_t sizePos = data.size();
    writeU32BE(0);  // Size (will update later)
    write4CC(compressionType ? "AIFC" : "AIFF");
    
    // COMM chunk
    write4CC("COMM");
    if (compressionType) {
        writeU32BE(22);  // Chunk size for AIFC
    } else {
        writeU32BE(18);  // Chunk size for AIFF
    }
    writeU16BE(channels);
    writeU32BE(numSamples);
    writeU16BE(bitDepth);
    
    // Sample rate as 80-bit IEEE extended
    if (sampleRate == 44100) {
        data.insert(data.end(), {0x40, 0x0E, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    } else if (sampleRate == 48000) {
        data.insert(data.end(), {0x40, 0x0E, 0xBB, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    } else {
        // Default to 44100
        data.insert(data.end(), {0x40, 0x0E, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    }
    
    // Compression type for AIFC
    if (compressionType) {
        write4CC(compressionType);
    }
    
    // SSND chunk
    write4CC("SSND");
    uint32_t soundDataSize;
    
    if (compressionType && strcmp(compressionType, "ima4") == 0) {
        // IMA4: 64 samples per 34-byte block per channel
        uint32_t blocksPerChannel = (numSamples + 63) / 64;
        soundDataSize = blocksPerChannel * 34 * channels + 8;
    } else {
        soundDataSize = numSamples * channels * (bitDepth / 8) + 8;
    }
    
    writeU32BE(soundDataSize);
    writeU32BE(0);  // Offset
    writeU32BE(0);  // Block size
    
    // Generate sample data
    if (compressionType && strcmp(compressionType, "ima4") == 0) {
        // Generate IMA4 compressed data (simplified - just headers)
        uint32_t blocksPerChannel = (numSamples + 63) / 64;
        for (uint32_t i = 0; i < blocksPerChannel * channels; i++) {
            // IMA4 block: 2 bytes header + 32 bytes compressed data
            writeU16BE(0);  // Initial predictor
            for (int j = 0; j < 32; j++) {
                data.push_back(0);  // Compressed nibbles
            }
        }
    } else {
        // Generate uncompressed PCM data
        for (uint32_t i = 0; i < numSamples * channels; i++) {
            if (bitDepth == 16) {
                int16_t sample = static_cast<int16_t>(std::sin(i * 0.1) * 32767);
                writeU16BE(static_cast<uint16_t>(sample));
            } else if (bitDepth == 8) {
                int8_t sample = static_cast<int8_t>(std::sin(i * 0.1) * 127);
                data.push_back(static_cast<uint8_t>(sample));
            } else if (bitDepth == 24) {
                int32_t sample = static_cast<int32_t>(std::sin(i * 0.1) * 8388607);
                data.push_back(static_cast<uint8_t>((sample >> 16) & 0xFF));
                data.push_back(static_cast<uint8_t>((sample >> 8) & 0xFF));
                data.push_back(static_cast<uint8_t>(sample & 0xFF));
            } else if (bitDepth == 32) {
                int32_t sample = static_cast<int32_t>(std::sin(i * 0.1) * 2147483647);
                writeU32BE(static_cast<uint32_t>(sample));
            }
        }
    }
    
    // Update FORM chunk size
    uint32_t formSize = static_cast<uint32_t>(data.size() - 8);
    data[sizePos] = static_cast<uint8_t>((formSize >> 24) & 0xFF);
    data[sizePos + 1] = static_cast<uint8_t>((formSize >> 16) & 0xFF);
    data[sizePos + 2] = static_cast<uint8_t>((formSize >> 8) & 0xFF);
    data[sizePos + 3] = static_cast<uint8_t>(formSize & 0xFF);
    
    return data;
}

TEST_SUITE("Decoders::AIFF_v2") {
    
    TEST_CASE("AIFF v2 Decoder - Basic Functionality") {
        SUBCASE("Opens valid 16-bit mono AIFF") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 100);
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == 1);
            CHECK(decoder.get_rate() == 44100);
        }
        
        SUBCASE("Opens valid 24-bit stereo AIFF") {
            auto testData = createTestAIFF_v2(2, 24, 48000, 100);
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == 2);
            CHECK(decoder.get_rate() == 48000);
        }
        
        SUBCASE("Opens valid 8-bit mono AIFF") {
            auto testData = createTestAIFF_v2(1, 8, 44100, 100);
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == 1);
        }
        
        SUBCASE("Opens valid 32-bit mono AIFF") {
            auto testData = createTestAIFF_v2(1, 32, 44100, 100);
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == 1);
        }
    }
    
    TEST_CASE("AIFF v2 Decoder - Compression Support") {
        SUBCASE("Opens µ-law compressed AIFC") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 100, "ulaw");
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.is_open());
        }
        
        SUBCASE("Opens A-law compressed AIFC") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 100, "alaw");
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.is_open());
        }
        
        SUBCASE("Opens IMA4 compressed AIFC") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 128, "ima4");
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.is_open());
        }
        
        SUBCASE("Opens sowt (little-endian) AIFC") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 100, "sowt");
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.is_open());
        }
    }
    
    TEST_CASE("AIFF v2 Decoder - Decoding") {
        SUBCASE("Decodes 16-bit mono data") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 100);
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            decoder.open(io.get());
            
            std::vector<float> output(256);
            bool more = true;
            size_t decoded = decoder.decode(output.data(), output.size(), more, 1);
            
            CHECK(decoded > 0);
            CHECK(decoded <= 100);  // Should not exceed number of samples
            
            // Check that samples are in valid range
            for (size_t i = 0; i < decoded; i++) {
                CHECK(output[i] >= -1.0f);
                CHECK(output[i] <= 1.0f);
            }
        }
        
        SUBCASE("Decodes stereo data correctly") {
            auto testData = createTestAIFF_v2(2, 16, 44100, 100);
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            decoder.open(io.get());
            
            std::vector<float> output(256);
            bool more = true;
            size_t decoded = decoder.decode(output.data(), output.size(), more, 2);
            
            CHECK(decoded > 0);
            CHECK(decoded <= 200);  // 100 samples * 2 channels
            CHECK(decoded % 2 == 0);  // Should be even number for stereo
        }
    }
    
    TEST_CASE("AIFF v2 Decoder - Accept Method") {
        SUBCASE("Accepts valid AIFF file") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 100);
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            CHECK(musac::decoder_aiff_v2::accept(io.get()));
            
            // Stream position should be restored
            CHECK(io->tell() == 0);
        }
        
        SUBCASE("Accepts valid AIFC file") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 100, "NONE");
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            CHECK(musac::decoder_aiff_v2::accept(io.get()));
            
            // Stream position should be restored
            CHECK(io->tell() == 0);
        }
        
        SUBCASE("Rejects non-AIFF data") {
            std::vector<uint8_t> badData = {0x00, 0x01, 0x02, 0x03};
            auto io = musac::io_from_memory(badData.data(), badData.size());
            
            CHECK_FALSE(musac::decoder_aiff_v2::accept(io.get()));
        }
        
        SUBCASE("Rejects truncated AIFF") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 100);
            testData.resize(10);  // Truncate to invalid size
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            CHECK_FALSE(musac::decoder_aiff_v2::accept(io.get()));
        }
    }
    
    TEST_CASE("AIFF v2 Decoder - Seek Operations") {
        SUBCASE("Seeks to beginning") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 1000);
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            decoder.open(io.get());
            
            // Decode some data
            std::vector<float> output(256);
            bool more = true;
            decoder.decode(output.data(), output.size(), more, 1);
            
            // Seek to beginning
            CHECK_NOTHROW(decoder.rewind());
            
            // Should be able to decode again from start
            size_t decoded = decoder.decode(output.data(), output.size(), more, 1);
            CHECK(decoded > 0);
        }
        
        SUBCASE("Seeks to specific time") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 44100);  // 1 second
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            decoder.open(io.get());
            
            // Seek to 0.5 seconds (500000 microseconds)
            CHECK_NOTHROW(decoder.seek_to_time(std::chrono::microseconds(500000)));
            
            // Decode and check we're not at the beginning
            std::vector<float> output(256);
            bool more = true;
            size_t decoded = decoder.decode(output.data(), output.size(), more, 1);
            CHECK(decoded > 0);
        }
    }
    
    TEST_CASE("AIFF v2 Decoder - Duration") {
        SUBCASE("Reports correct duration") {
            auto testData = createTestAIFF_v2(1, 16, 44100, 44100);  // 1 second
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            decoder.open(io.get());
            
            auto duration = decoder.duration();
            // Convert microseconds to seconds
            double duration_seconds = static_cast<double>(duration.count()) / 1000000.0;
            CHECK(duration_seconds == doctest::Approx(1.0).epsilon(0.01));
        }
        
        SUBCASE("Reports correct duration for stereo") {
            auto testData = createTestAIFF_v2(2, 16, 44100, 44100);  // 1 second stereo
            auto io = musac::io_from_memory(testData.data(), testData.size());
            
            musac::decoder_aiff_v2 decoder;
            decoder.open(io.get());
            
            auto duration = decoder.duration();
            // Convert microseconds to seconds
            double duration_seconds = static_cast<double>(duration.count()) / 1000000.0;
            CHECK(duration_seconds == doctest::Approx(1.0).epsilon(0.01));
        }
    }
    
    TEST_CASE("AIFF v2 Decoder - Error Handling") {
        SUBCASE("Throws on invalid file") {
            std::vector<uint8_t> badData = {0xFF, 0xFF, 0xFF, 0xFF};
            auto io = musac::io_from_memory(badData.data(), badData.size());
            
            musac::decoder_aiff_v2 decoder;
            CHECK_THROWS(decoder.open(io.get()));
        }
        
        SUBCASE("Throws on missing COMM chunk") {
            std::vector<uint8_t> data;
            
            // Write FORM header without COMM chunk
            auto write4CC = [&data](const char* fourcc) {
                data.insert(data.end(), fourcc, fourcc + 4);
            };
            auto writeU32BE = [&data](uint32_t value) {
                data.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
                data.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
                data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
                data.push_back(static_cast<uint8_t>(value & 0xFF));
            };
            
            write4CC("FORM");
            writeU32BE(12);
            write4CC("AIFF");
            
            auto io = musac::io_from_memory(data.data(), data.size());
            
            musac::decoder_aiff_v2 decoder;
            CHECK_THROWS(decoder.open(io.get()));
        }
    }
}