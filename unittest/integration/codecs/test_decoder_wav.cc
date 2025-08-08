#include <doctest/doctest.h>
#include <musac/codecs/decoder_drwav.hh>
#include <musac/sdk/io_stream.hh>
#include <vector>
#include <cstring>
#include <cmath>

// Helper to create a minimal WAV file in memory
std::vector<uint8_t> createTestWAV(uint16_t channels = 1,
                                  uint16_t bitDepth = 16, 
                                  uint32_t sampleRate = 44100,
                                  uint32_t numSamples = 100) {
    std::vector<uint8_t> data;
    
    auto writeU32LE = [&data](uint32_t value) {
        data.push_back(static_cast<uint8_t>(value & 0xFF));
        data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        data.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    };
    
    auto writeU16LE = [&data](uint16_t value) {
        data.push_back(static_cast<uint8_t>(value & 0xFF));
        data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    };
    
    // RIFF header
    data.insert(data.end(), {'R', 'I', 'F', 'F'});
    size_t fileSizePos = data.size();
    writeU32LE(0);  // File size (will update later)
    data.insert(data.end(), {'W', 'A', 'V', 'E'});
    
    // fmt chunk
    data.insert(data.end(), {'f', 'm', 't', ' '});
    writeU32LE(16);  // Chunk size
    writeU16LE(1);   // Audio format (1 = PCM)
    writeU16LE(channels);
    writeU32LE(sampleRate);
    writeU32LE(sampleRate * channels * (bitDepth / 8));  // Byte rate
    writeU16LE(channels * (bitDepth / 8));  // Block align
    writeU16LE(bitDepth);
    
    // data chunk
    data.insert(data.end(), {'d', 'a', 't', 'a'});
    uint32_t dataSize = numSamples * channels * (bitDepth / 8);
    writeU32LE(dataSize);
    
    // Sample data (silence)
    size_t oldSize = data.size();
    data.resize(oldSize + dataSize, 0);
    
    // Update file size
    uint32_t fileSize = static_cast<uint32_t>(data.size() - 8);
    data[fileSizePos] = static_cast<uint8_t>(fileSize & 0xFF);
    data[fileSizePos + 1] = static_cast<uint8_t>((fileSize >> 8) & 0xFF);
    data[fileSizePos + 2] = static_cast<uint8_t>((fileSize >> 16) & 0xFF);
    data[fileSizePos + 3] = static_cast<uint8_t>((fileSize >> 24) & 0xFF);
    
    return data;
}

TEST_SUITE("Codecs::DecoderWAV") {
    TEST_CASE("Open valid WAV file") {
        auto wavData = createTestWAV(2, 16, 44100, 1000);
        auto io = musac::io_from_memory(wavData.data(), wavData.size());
        REQUIRE(io != nullptr);
        
        musac::decoder_drwav decoder;
        
        SUBCASE("Successfully opens") {
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == 2);
            CHECK(decoder.get_rate() == 44100);
        }
    }
    
    TEST_CASE("Decode WAV data") {
        auto wavData = createTestWAV(1, 16, 44100, 100);
        auto io = musac::io_from_memory(wavData.data(), wavData.size());
        REQUIRE(io != nullptr);
        
        musac::decoder_drwav decoder;
        REQUIRE_NOTHROW(decoder.open(io.get()));
        
        SUBCASE("Decode samples") {
            float buffer[100];
            bool call_again = true;
            size_t decoded = decoder.decode(buffer, 100, call_again, 2);
            
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
                size_t decoded = decoder.decode(buffer, 50, call_again, 2);
                if (decoded == 0 && call_again) {
                    // Prevent infinite loop if decoder returns 0 but call_again is true
                    FAIL("Decoder returned 0 samples but call_again is true");
                    break;
                }
                allSamples.insert(allSamples.end(), buffer, buffer + decoded);
            }
            
            // Should have decoded some samples
            CHECK(allSamples.size() > 0);
            INFO("Decoded " << allSamples.size() << " samples from 100 input samples");
        }
    }
    
    TEST_CASE("Different WAV formats") {
        SUBCASE("8-bit mono") {
            auto wavData = createTestWAV(1, 8, 22050, 50);
            auto io = musac::io_from_memory(wavData.data(), wavData.size());
            
            musac::decoder_drwav decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.get_channels() == 1);
            CHECK(decoder.get_rate() == 22050);
        }
        
        SUBCASE("24-bit stereo") {
            auto wavData = createTestWAV(2, 24, 48000, 50);
            auto io = musac::io_from_memory(wavData.data(), wavData.size());
            
            musac::decoder_drwav decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            CHECK(decoder.get_channels() == 2);
            CHECK(decoder.get_rate() == 48000);
        }
    }
    
    TEST_CASE("Invalid WAV data") {
        SUBCASE("Not WAV format") {
            uint8_t badData[] = {'F', 'O', 'R', 'M', 0, 0, 0, 0, 'A', 'I', 'F', 'F'};
            auto io = musac::io_from_memory(badData, sizeof(badData));
            
            musac::decoder_drwav decoder;
            CHECK_THROWS(decoder.open(io.get()));
        }
        
        SUBCASE("Truncated file") {
            auto wavData = createTestWAV();
            wavData.resize(20);  // Cut off most of the file
            
            auto io = musac::io_from_memory(wavData.data(), wavData.size());
            
            musac::decoder_drwav decoder;
            CHECK_THROWS(decoder.open(io.get()));
        }
    }
    
    TEST_CASE("Seeking in WAV") {
        auto wavData = createTestWAV(2, 16, 44100, 44100);  // 1 second
        auto io = musac::io_from_memory(wavData.data(), wavData.size());
        
        musac::decoder_drwav decoder;
        REQUIRE_NOTHROW(decoder.open(io.get()));
        
        SUBCASE("Seek to time") {
            // Seek to 0.5 seconds
            CHECK(decoder.seek_to_time(std::chrono::milliseconds(500)));
            
            // Decode and check position
            float buffer[100];
            bool call_again;
            size_t decoded_samples = decoder.decode(buffer, 100, call_again, 2);
            CHECK(decoded_samples > 0);
            
            // Should have data remaining
            CHECK(call_again);
        }
    }
    
    TEST_CASE("Duration calculation") {
        SUBCASE("1 second mono") {
            auto wavData = createTestWAV(1, 16, 44100, 44100);
            auto io = musac::io_from_memory(wavData.data(), wavData.size());
            
            musac::decoder_drwav decoder;
            REQUIRE_NOTHROW(decoder.open(io.get()));
            
            auto duration = decoder.duration();
            CHECK(duration == std::chrono::seconds(1));
        }
        
        SUBCASE("500ms stereo") {
            auto wavData = createTestWAV(2, 16, 48000, 24000);
            auto io = musac::io_from_memory(wavData.data(), wavData.size());
            
            musac::decoder_drwav decoder;
            REQUIRE_NOTHROW(decoder.open(io.get()));
            
            auto duration = decoder.duration();
            CHECK(duration == std::chrono::milliseconds(500));
        }
    }
    
    TEST_CASE("Rewind functionality") {
        auto wavData = createTestWAV(1, 16, 44100, 200);
        auto io = musac::io_from_memory(wavData.data(), wavData.size());
        
        musac::decoder_drwav decoder;
        REQUIRE_NOTHROW(decoder.open(io.get()));
        
        // Decode some data
        float buffer[100];
        bool call_again;
        size_t decoded_count = decoder.decode(buffer, 100, call_again, 2);
        CHECK(decoded_count > 0);
        
        // Rewind
        CHECK(decoder.rewind());
        
        // Decode again - should get same data
        float buffer2[100];
        size_t decoded_count2 = decoder.decode(buffer2, 100, call_again, 2);
        CHECK(decoded_count2 > 0);
        
        // Compare (should be identical)
        for (int i = 0; i < 100; i++) {
            CHECK(buffer[i] == buffer2[i]);
        }
    }
}