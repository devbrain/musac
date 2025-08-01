#include <doctest/doctest.h>
#include <musac/codecs/decoder_aiff.hh>
#include <musac/sdk/io_stream.h>
#include <vector>
#include <cstring>
#include <cmath>

// Helper to create a minimal AIFF file in memory
std::vector<uint8_t> createTestAIFF(uint16_t channels = 1, 
                                   uint16_t bitDepth = 16,
                                   uint32_t /*sampleRate*/ = 44100,
                                   uint32_t numSamples = 100) {
    std::vector<uint8_t> data;
    
    auto writeU32BE = [&data](uint32_t value) {
        data.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
        data.push_back((value >> 16) & 0xFF);
        data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(value & 0xFF));
    };
    
    auto writeU16BE = [&data](uint16_t value) {
        data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(value & 0xFF));
    };
    
    // FORM chunk
    writeU32BE(0x464F524D);  // "FORM"
    // size_t sizePos = data.size();  // unused
    writeU32BE(0);  // Size (will update later)
    writeU32BE(0x41494646);  // "AIFF"
    
    // COMM chunk
    writeU32BE(0x434F4D4D);  // "COMM"
    writeU32BE(18);  // Chunk size
    writeU16BE(channels);
    writeU32BE(numSamples);
    writeU16BE(bitDepth);
    
    // Sample rate as 80-bit IEEE extended (simplified)
    // 44100 Hz = 0x400EAC44 00000000 0000
    data.insert(data.end(), {0x40, 0x0E, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    
    // SSND chunk
    writeU32BE(0x53534E44);  // "SSND"
    uint32_t soundDataSize = numSamples * channels * (bitDepth / 8) + 8;
    writeU32BE(soundDataSize);
    writeU32BE(0);  // Offset
    writeU32BE(0);  // Block size
    
    // Sample data (silence)
    size_t sampleBytes = numSamples * channels * (bitDepth / 8);
    data.resize(data.size() + sampleBytes, 0);
    
    // Update FORM size
    uint32_t formSize = static_cast<uint32_t>(data.size() - 8);
    data[4] = static_cast<uint8_t>((formSize >> 24) & 0xFF);
    data[5] = static_cast<uint8_t>((formSize >> 16) & 0xFF);
    data[6] = static_cast<uint8_t>((formSize >> 8) & 0xFF);
    data[7] = static_cast<uint8_t>(formSize & 0xFF);
    
    return data;
}

TEST_SUITE("Codecs::DecoderAIFF") {
    TEST_CASE("Open valid AIFF file") {
        auto aiffData = createTestAIFF(2, 16, 44100, 1000);
        auto io = musac::io_from_memory(aiffData.data(), aiffData.size());
        REQUIRE(io != nullptr);
        
        musac::decoder_aiff decoder;
        
        SUBCASE("Successfully opens") {
            CHECK(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == 1); // Converted to mono
            CHECK(decoder.get_rate() == 44100);
        }
    }
    
    TEST_CASE("Decode AIFF data") {
        auto aiffData = createTestAIFF(1, 16, 44100, 100);
        auto io = musac::io_from_memory(aiffData.data(), aiffData.size());
        REQUIRE(io != nullptr);
        
        musac::decoder_aiff decoder;
        REQUIRE(decoder.open(io.get()));
        
        SUBCASE("Decode samples") {
            float buffer[100];
            bool call_again = true;
            unsigned int decoded = decoder.decode(buffer, 100, call_again, 2);
            
            CHECK(decoded > 0);
            CHECK(decoded <= 100);
            
            // Should be silence (zeros)
            for (unsigned int i = 0; i < decoded; i++) {
                CHECK(std::abs(buffer[i]) < 0.001f);
            }
        }
        
        SUBCASE("Decode all data") {
            std::vector<float> allSamples;
            float buffer[50];
            bool call_again = true;
            
            while (call_again) {
                unsigned int decoded = decoder.decode(buffer, 50, call_again, 2);
                if (decoded == 0 && call_again) {
                    // Prevent infinite loop if decoder returns 0 but call_again is true
                    FAIL("Decoder returned 0 samples but call_again is true");
                    break;
                }
                allSamples.insert(allSamples.end(), buffer, buffer + decoded);
            }
            
            // Should have decoded some samples (might be resampled or channel converted)
            CHECK(allSamples.size() > 0);
            INFO("Decoded " << allSamples.size() << " samples from 100 input samples");
        }
    }
    
    TEST_CASE("Invalid AIFF data") {
        SUBCASE("Not AIFF format") {
            uint8_t badData[] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'};
            auto io = musac::io_from_memory(badData, sizeof(badData));
            
            musac::decoder_aiff decoder;
            CHECK_FALSE(decoder.open(io.get()));
        }
        
        SUBCASE("Truncated file") {
            auto aiffData = createTestAIFF();
            aiffData.resize(20); // Cut off most of the file
            
            auto io = musac::io_from_memory(aiffData.data(), aiffData.size());
            
            musac::decoder_aiff decoder;
            CHECK_FALSE(decoder.open(io.get()));
        }
        
        SUBCASE("Missing COMM chunk") {
            std::vector<uint8_t> data;
            
            // FORM header only
            data.insert(data.end(), {'F', 'O', 'R', 'M'});
            data.insert(data.end(), {0, 0, 0, 12});
            data.insert(data.end(), {'A', 'I', 'F', 'F'});
            
            auto io = musac::io_from_memory(data.data(), data.size());
            
            musac::decoder_aiff decoder;
            CHECK_FALSE(decoder.open(io.get()));
        }
    }
    
    TEST_CASE("8SVX format support") {
        std::vector<uint8_t> data;
        
        auto writeU32BE = [&data](uint32_t value) {
            data.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
            data.push_back((value >> 16) & 0xFF);
            data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
            data.push_back(static_cast<uint8_t>(value & 0xFF));
        };
        
        auto writeU16BE = [&data](uint16_t value) {
            data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
            data.push_back(static_cast<uint8_t>(value & 0xFF));
        };
        
        // FORM chunk
        writeU32BE(0x464F524D);  // "FORM"
        writeU32BE(100);  // Size
        writeU32BE(0x38535658);  // "8SVX"
        
        // VHDR chunk
        writeU32BE(0x56484452);  // "VHDR"
        writeU32BE(20);  // Size
        
        // VHDR data
        data.resize(data.size() + 12, 0);  // Skip first 12 bytes
        writeU16BE(22050);  // Sample rate
        data.resize(data.size() + 6, 0);  // Rest of VHDR
        
        // BODY chunk
        writeU32BE(0x424F4459);  // "BODY"
        writeU32BE(50);  // Size
        
        // Sample data
        data.resize(data.size() + 50, 128);  // 8-bit centered at 128
        
        auto io = musac::io_from_memory(data.data(), data.size());
        
        musac::decoder_aiff decoder;
        CHECK(decoder.open(io.get()));
        CHECK(decoder.get_channels() == 1);
        CHECK(decoder.get_rate() == 44100); // Converted from 22050
    }
    
    TEST_CASE("Rewind functionality") {
        auto aiffData = createTestAIFF(1, 16, 44100, 200);
        auto io = musac::io_from_memory(aiffData.data(), aiffData.size());
        
        musac::decoder_aiff decoder;
        REQUIRE(decoder.open(io.get()));
        
        // Decode some data
        float buffer[100];
        bool call_again;
        unsigned int decoded_count = decoder.decode(buffer, 100, call_again, 2);
        CHECK(decoded_count > 0);
        
        // Rewind
        CHECK(decoder.rewind());
        
        // Decode again - should get same data
        float buffer2[100];
        unsigned int decoded_count2 = decoder.decode(buffer2, 100, call_again, 2);
        CHECK(decoded_count2 > 0);
        
        // Compare buffers (should be identical)
        for (int i = 0; i < 100; i++) {
            CHECK(buffer[i] == buffer2[i]);
        }
    }
}