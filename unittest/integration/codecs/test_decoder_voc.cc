#include <doctest/doctest.h>
#include <musac/codecs/decoder_voc.hh>
#include <musac/sdk/io_stream.hh>
#include <vector>
#include <cstring>
#include <cmath>
#include "golden_data/voc_test_data_full.h"

// Helper to create a minimal VOC file in memory
std::vector<uint8_t> createTestVOC(uint8_t sampleRate = 256 - 45, // ~22050 Hz
                                  uint32_t numSamples = 100) {
    std::vector<uint8_t> data;
    
    // VOC header
    const char* signature = "Creative Voice File\x1a";
    data.insert(data.end(), signature, signature + 20);
    
    // Header data
    data.push_back(0x1A);  // Data block offset (LSB)
    data.push_back(0x00);  // Data block offset (MSB)
    data.push_back(0x0A);  // Version (LSB) 
    data.push_back(0x01);  // Version (MSB) - Version 1.10
    data.push_back(0x29);  // Checksum (LSB)
    data.push_back(0x11);  // Checksum (MSB)
    
    // Type 1 data block (8-bit mono)
    data.push_back(0x01);  // Block type
    
    // Block size (3 bytes, little-endian)
    uint32_t blockSize = numSamples + 2;  // samples + rate + pack bytes
    data.push_back(static_cast<uint8_t>(blockSize & 0xFF));
    data.push_back(static_cast<uint8_t>((blockSize >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>((blockSize >> 16) & 0xFF));
    
    data.push_back(sampleRate);  // Sample rate code
    data.push_back(0x00);  // Pack byte (8-bit unpacked)
    
    // Sample data (centered at 128 for unsigned 8-bit)
    for (uint32_t i = 0; i < numSamples; i++) {
        data.push_back(128);  // Silence
    }
    
    // Terminator block
    data.push_back(0x00);  // Block type 0 = terminator
    
    return data;
}

TEST_SUITE("Codecs::DecoderVOC") {
    TEST_CASE("Open valid VOC file") {
        auto vocData = createTestVOC(256 - 45, 1000);
        auto io = musac::io_from_memory(vocData.data(), vocData.size());
        REQUIRE(io != nullptr);
        
        musac::decoder_voc decoder;
        
        SUBCASE("Successfully opens") {
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == 1);  // Converted to mono
            CHECK(decoder.get_rate() == 44100);  // Converted to standard rate
        }
    }
    
    TEST_CASE("Decode VOC data") {
        auto vocData = createTestVOC(256 - 45, 100);
        auto io = musac::io_from_memory(vocData.data(), vocData.size());
        REQUIRE(io != nullptr);
        
        musac::decoder_voc decoder;
        REQUIRE_NOTHROW(decoder.open(io.get()));
        
        SUBCASE("Decode samples") {
            float buffer[100];
            bool call_again = true;
            size_t decoded = decoder.decode(buffer, 100, call_again, 2);
            
            CHECK(decoded > 0);
            
            // Should be silence (zeros after conversion from U8 128)
            for (unsigned int i = 0; i < decoded && i < 10; i++) {
                CHECK(std::abs(buffer[i]) < 0.1f);
            }
        }
        
        SUBCASE("Decode all data") {
            std::vector<float> allSamples;
            float buffer[50];
            bool call_again = true;
            
            while (call_again) {
                size_t decoded = decoder.decode(buffer, 50, call_again, 2);
                if (decoded == 0 && call_again) {
                    // Prevent infinite loop if decoder returns 0 but call_again is true
                    FAIL("Decoder returned 0 samples but call_again is true");
                    break;
                }
                allSamples.insert(allSamples.end(), buffer, buffer + decoded);
            }
            
            // Should have decoded samples (may be resampled)
            CHECK(allSamples.size() > 0);
        }
    }
    
    TEST_CASE("Real VOC file") {
        // Use embedded VOC data instead of file
        auto io = musac::io_from_memory(voc_test_data, voc_test_data_len);
        REQUIRE(io != nullptr);
        
        musac::decoder_voc decoder;
        
        CHECK_NOTHROW(decoder.open(io.get()));
        CHECK(decoder.is_open());
        CHECK(decoder.get_channels() == 1);  // VOC is mono
        CHECK(decoder.get_rate() == 44100);  // Converted to standard rate
        
        // Decode some samples
        float buffer[1000];
        bool call_again = true;
        size_t total_decoded = 0;
        
        while (call_again && total_decoded < 10000) {
            size_t decoded = decoder.decode(buffer, 1000, call_again, 2);
            if (decoded == 0 && call_again) {
                FAIL("Decoder returned 0 samples but call_again is true");
                break;
            }
            total_decoded += decoded;
        }
        
        CHECK(total_decoded > 0);
    }
    
    TEST_CASE("Invalid VOC data") {
        SUBCASE("Not VOC format") {
            uint8_t badData[] = {'R', 'I', 'F', 'F', 0, 0, 0, 0};
            auto io = musac::io_from_memory(badData, sizeof(badData));
            
            musac::decoder_voc decoder;
            CHECK_THROWS(decoder.open(io.get()));
        }
        
        SUBCASE("Truncated file") {
            auto vocData = createTestVOC();
            vocData.resize(10);  // Cut off most of the file
            
            auto io = musac::io_from_memory(vocData.data(), vocData.size());
            
            musac::decoder_voc decoder;
            CHECK_THROWS(decoder.open(io.get()));
        }
        
        SUBCASE("Invalid sample rate") {
            auto vocData = createTestVOC(0, 100);  // Rate code 0 is invalid
            auto io = musac::io_from_memory(vocData.data(), vocData.size());
            
            musac::decoder_voc decoder;
            CHECK_THROWS(decoder.open(io.get()));
        }
    }
    
    TEST_CASE("VOC extended format") {
        std::vector<uint8_t> data;
        
        // VOC header
        const char* signature = "Creative Voice File\x1a";
        data.insert(data.end(), signature, signature + 20);
        data.insert(data.end(), {0x1A, 0x00, 0x0A, 0x01, 0x29, 0x11});
        
        // Type 8 extended block
        data.push_back(0x08);  // Block type
        data.push_back(0x04);  // Size LSB
        data.push_back(0x00);  // Size
        data.push_back(0x00);  // Size MSB
        
        // Extended block data
        uint16_t timeConstant = 65536 - (256000000L / 22050);
        data.push_back(static_cast<uint8_t>(timeConstant & 0xFF));
        data.push_back(static_cast<uint8_t>((timeConstant >> 8) & 0xFF));
        data.push_back(0x00);  // Pack byte (8-bit)
        data.push_back(0x00);  // Mode (mono)
        
        // Type 1 data block following extended
        data.push_back(0x01);  // Block type
        data.push_back(0x32);  // Size LSB (50 bytes + 2)
        data.push_back(0x00);
        data.push_back(0x00);
        data.push_back(256 - 45);  // Rate (ignored due to extended)
        data.push_back(0x00);  // Pack
        
        // Sample data
        for (int i = 0; i < 50; i++) {
            data.push_back(128);
        }
        
        // Terminator
        data.push_back(0x00);
        
        auto io = musac::io_from_memory(data.data(), data.size());
        musac::decoder_voc decoder;
        
        CHECK_NOTHROW(decoder.open(io.get()));
        CHECK(decoder.get_channels() == 1);
    }
    
    TEST_CASE("Rewind functionality") {
        auto vocData = createTestVOC(256 - 45, 200);
        auto io = musac::io_from_memory(vocData.data(), vocData.size());
        
        musac::decoder_voc decoder;
        REQUIRE_NOTHROW(decoder.open(io.get()));
        
        // Decode some data
        float buffer[100];
        bool call_again;
        size_t decoded_count = decoder.decode(buffer, 100, call_again, 2);
        CHECK(decoded_count > 0);
        
        // Rewind
        CHECK(decoder.rewind());
        
        // Decode again
        float buffer2[100];
        size_t decoded_count2 = decoder.decode(buffer2, 100, call_again, 2);
        CHECK(decoded_count2 > 0);
        
        // Should get same data
        for (int i = 0; i < 100; i++) {
            CHECK(buffer[i] == buffer2[i]);
        }
    }
}