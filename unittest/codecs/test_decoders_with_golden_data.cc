#include <doctest/doctest.h>
#include <musac/codecs/decoder_aiff.hh>
#include <musac/codecs/decoder_voc.hh>
#include <musac/codecs/decoder_drwav.hh>
#include <musac/codecs/decoder_cmf.hh>
#include <musac/codecs/decoder_seq.hh>
#include <musac/codecs/decoder_opb.hh>
#include <musac/codecs/decoder_vgm.hh>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/io_stream.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <functional>
#include <memory>
#include <iostream>

// Include generated test data headers
// These are in the root of the build directory
#include "test_aiff_data.h"
#include "test_voc_data.h"
#include "test_wav_data.h"
#include "test_cmf_data.h"
#include "test_mid_data.h"
#include "test_mus_data.h"
#include "test_opb_data.h"
#include "test_vgz_data.h"
#include "test_xmi_data.h"

namespace {
    // Helper function to compare float arrays with tolerance
    bool compareFloatArrays(const float* expected, const float* actual, size_t count, float tolerance = 0.001f) {
        int failures = 0;
        float max_diff = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            float diff = std::abs(expected[i] - actual[i]);
            if (diff > max_diff) max_diff = diff;
            if (diff > tolerance) {
                failures++;
                // Debug output to see the differences
                if (failures <= 10) {
                    std::cerr << "Sample " << i << ": expected=" << expected[i] 
                             << ", actual=" << actual[i] 
                             << ", diff=" << diff << std::endl;
                }
            }
        }
        if (failures > 0) {
            std::cerr << "Total failures: " << failures << "/" << count 
                     << " (" << (100.0 * static_cast<double>(failures) / static_cast<double>(count)) << "%)" 
                     << ", max diff: " << max_diff << std::endl;
            return false;
        }
        return true;
    }
    
    // Helper to decode all data from a decoder
    std::vector<float> decodeAll(musac::decoder& dec, unsigned int device_channels, size_t max_samples = 0) {
        std::vector<float> result;
        const size_t chunk_size = 4096;
        std::vector<float> chunk(chunk_size);
        bool call_again = true;
        
        while (call_again) {
            size_t to_decode = chunk_size;
            if (max_samples > 0 && result.size() + to_decode > max_samples) {
                to_decode = max_samples - result.size();
                if (to_decode == 0) break;
            }
            
            unsigned int decoded = dec.decode(chunk.data(), static_cast<unsigned int>(to_decode), call_again, device_channels);
            if (decoded > 0) {
                result.insert(result.end(), chunk.begin(), chunk.begin() + decoded);
            }
            
            // Stop if we've reached the limit
            if (max_samples > 0 && result.size() >= max_samples) {
                break;
            }
        }
        
        return result;
    }
}


TEST_SUITE("Decoders::GoldenData") {
    TEST_CASE("AIFF Decoder - Golden Data Test") {
        auto io = musac::io_from_memory(test16_aiff_input, test16_aiff_input_size);
        REQUIRE(io != nullptr);
        
        musac::decoder_aiff decoder;
        
        SUBCASE("Opens correctly") {
            CHECK(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == test16_aiff_channels);
            CHECK(decoder.get_rate() == test16_aiff_rate);
        }
        
        SUBCASE("Decodes to expected output") {
            REQUIRE(decoder.open(io.get()));
            
            auto decoded = decodeAll(decoder, test16_aiff_channels);
            
            // For non-limited outputs, we should get exactly the expected data
            if (!test16_aiff_output_limited) {
                CHECK(decoded.size() == test16_aiff_output_size);
                CHECK(compareFloatArrays(test16_aiff_output, decoded.data(), 
                                       std::min(decoded.size(), test16_aiff_output_size)));
            } else {
                // For limited outputs, we should get at least some data
                CHECK(decoded.size() > 0);
                CHECK(decoded.size() <= test16_aiff_output_size);
                // Compare what we got
                CHECK(compareFloatArrays(test16_aiff_output, decoded.data(), decoded.size()));
            }
        }
    }
    
    TEST_CASE("VOC Decoder - Golden Data Test") {
        auto io = musac::io_from_memory(file_1_voc_input, file_1_voc_input_size);
        REQUIRE(io != nullptr);
        
        musac::decoder_voc decoder;
        
        SUBCASE("Opens correctly") {
            CHECK(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == file_1_voc_channels);
            CHECK(decoder.get_rate() == file_1_voc_rate);
        }
        
        SUBCASE("Decodes to expected output") {
            REQUIRE(decoder.open(io.get()));
            
            auto decoded = decodeAll(decoder, file_1_voc_channels);
            
            if (!file_1_voc_output_limited) {
                CHECK(decoded.size() == file_1_voc_output_size);
                CHECK(compareFloatArrays(file_1_voc_output, decoded.data(), 
                                       std::min(decoded.size(), file_1_voc_output_size), 0.2f));
            } else {
                CHECK(decoded.size() > 0);
                CHECK(decoded.size() <= file_1_voc_output_size);
                CHECK(compareFloatArrays(file_1_voc_output, decoded.data(), decoded.size()));
            }
        }
    }
    
    TEST_CASE("WAV Decoder - Golden Data Test") {
        auto io = musac::io_from_memory(soundcard_wav_input, soundcard_wav_input_size);
        REQUIRE(io != nullptr);
        
        musac::decoder_drwav decoder;
        
        SUBCASE("Opens correctly") {
            CHECK(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == soundcard_wav_channels);
            CHECK(decoder.get_rate() == soundcard_wav_rate);
        }
        
        SUBCASE("Decodes to expected output") {
            REQUIRE(decoder.open(io.get()));
            
            // For WAV files, we decode up to the size of our golden data
            const size_t decode_limit = soundcard_wav_output_size;
            std::vector<float> buffer(decode_limit);
            bool call_again;
            
            unsigned int decoded = decoder.decode(buffer.data(), decode_limit, call_again, soundcard_wav_channels);
            CHECK(decoded > 0);
            CHECK(decoded <= decode_limit);
            
            // Compare what we decoded
            CHECK(compareFloatArrays(soundcard_wav_output, buffer.data(), decoded));
        }
    }
    
    TEST_CASE("CMF Decoder - Golden Data Test") {
        auto io = musac::io_from_memory(brix_cmf_input, brix_cmf_input_size);
        REQUIRE(io != nullptr);
        
        musac::decoder_cmf decoder;
        
        SUBCASE("Opens correctly") {
            CHECK(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == brix_cmf_channels);
            CHECK(decoder.get_rate() == brix_cmf_rate);
        }
        
        SUBCASE("Decodes to expected output") {
            REQUIRE(decoder.open(io.get()));
            
            // For synthesizer formats that were limited, only decode up to the output size
            size_t decode_limit = brix_cmf_output_limited ? brix_cmf_output_size : 0;
            auto decoded = decodeAll(decoder, brix_cmf_channels, decode_limit);
            
            CHECK(decoded.size() > 0);
            CHECK(decoded.size() >= brix_cmf_output_size);
            // Compare the golden data we have
            CHECK(compareFloatArrays(brix_cmf_output, decoded.data(), 
                                   std::min(brix_cmf_output_size, decoded.size()), 0.01f));
        }
    }
    
    TEST_CASE("MIDI Decoder - Golden Data Test") {
        auto io = musac::io_from_memory(simon_mid_input, simon_mid_input_size);
        REQUIRE(io != nullptr);
        
        musac::decoder_seq decoder;
        
        SUBCASE("Opens correctly") {
            CHECK(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == simon_mid_channels);
            CHECK(decoder.get_rate() == simon_mid_rate);
        }
        
        SUBCASE("Decodes to expected output") {
            REQUIRE(decoder.open(io.get()));
            
            size_t decode_limit = simon_mid_output_limited ? simon_mid_output_size : 0;
            auto decoded = decodeAll(decoder, simon_mid_channels, decode_limit);
            
            CHECK(decoded.size() > 0);
            CHECK(decoded.size() >= simon_mid_output_size);
            CHECK(compareFloatArrays(simon_mid_output, decoded.data(), 
                                   std::min(simon_mid_output_size, decoded.size()), 0.01f));
        }
    }
    
    TEST_CASE("MUS Decoder - Golden Data Test") {
        auto io = musac::io_from_memory(doom_mus_input, doom_mus_input_size);
        REQUIRE(io != nullptr);
        
        musac::decoder_seq decoder;
        
        SUBCASE("Opens correctly") {
            CHECK(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == doom_mus_channels);
            CHECK(decoder.get_rate() == doom_mus_rate);
        }
        
        SUBCASE("Decodes to expected output") {
            REQUIRE(decoder.open(io.get()));
            
            size_t decode_limit = doom_mus_output_limited ? doom_mus_output_size : 0;
            auto decoded = decodeAll(decoder, doom_mus_channels, decode_limit);
            
            CHECK(decoded.size() > 0);
            CHECK(decoded.size() >= doom_mus_output_size);
            CHECK(compareFloatArrays(doom_mus_output, decoded.data(), 
                                   std::min(doom_mus_output_size, decoded.size()), 0.01f));
        }
    }
    
    TEST_CASE("OPB Decoder - Golden Data Test") {
        auto io = musac::io_from_memory(doom_opb_input, doom_opb_input_size);
        REQUIRE(io != nullptr);
        
        musac::decoder_opb decoder;
        
        SUBCASE("Opens correctly") {
            CHECK(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == doom_opb_channels);
            CHECK(decoder.get_rate() == doom_opb_rate);
        }
        
        SUBCASE("Decodes to expected output") {
            REQUIRE(decoder.open(io.get()));
            
            size_t decode_limit = doom_opb_output_limited ? doom_opb_output_size : 0;
            auto decoded = decodeAll(decoder, doom_opb_channels, decode_limit);
            
            CHECK(decoded.size() > 0);
            CHECK(decoded.size() >= doom_opb_output_size);
            CHECK(compareFloatArrays(doom_opb_output, decoded.data(), 
                                   std::min(doom_opb_output_size, decoded.size()), 0.01f));
        }
    }
    
    TEST_CASE("VGM Decoder - Golden Data Test") {
        auto io = musac::io_from_memory(vgm_vgz_input, vgm_vgz_input_size);
        REQUIRE(io != nullptr);
        
        musac::decoder_vgm decoder;
        
        SUBCASE("Opens correctly") {
            CHECK(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == vgm_vgz_channels);
            CHECK(decoder.get_rate() == vgm_vgz_rate);
        }
        
        SUBCASE("Decodes to expected output") {
            REQUIRE(decoder.open(io.get()));
            
            size_t decode_limit = vgm_vgz_output_limited ? vgm_vgz_output_size : 0;
            auto decoded = decodeAll(decoder, vgm_vgz_channels, decode_limit);
            
            CHECK(decoded.size() > 0);
            CHECK(decoded.size() >= vgm_vgz_output_size);
            CHECK(compareFloatArrays(vgm_vgz_output, decoded.data(), 
                                   std::min(vgm_vgz_output_size, decoded.size()), 0.01f));
        }
    }
    
    TEST_CASE("XMI Decoder - Golden Data Test") {
        auto io = musac::io_from_memory(GCOMP1_XMI_input, GCOMP1_XMI_input_size);
        REQUIRE(io != nullptr);
        
        musac::decoder_seq decoder;
        
        SUBCASE("Opens correctly") {
            CHECK(decoder.open(io.get()));
            CHECK(decoder.is_open());
            CHECK(decoder.get_channels() == GCOMP1_XMI_channels);
            CHECK(decoder.get_rate() == GCOMP1_XMI_rate);
        }
        
        SUBCASE("Decodes to expected output") {
            REQUIRE(decoder.open(io.get()));
            
            size_t decode_limit = GCOMP1_XMI_output_limited ? GCOMP1_XMI_output_size : 0;
            auto decoded = decodeAll(decoder, GCOMP1_XMI_channels, decode_limit);
            
            CHECK(decoded.size() > 0);
            CHECK(decoded.size() >= GCOMP1_XMI_output_size);
            CHECK(compareFloatArrays(GCOMP1_XMI_output, decoded.data(), 
                                   std::min(GCOMP1_XMI_output_size, decoded.size()), 0.01f));
        }
    }
    
    TEST_CASE("Decoder Regression Tests") {
        SUBCASE("All decoders produce consistent output") {
            // This test ensures that decoders produce the same output
            // when given the same input multiple times
            
            struct TestCase {
                const uint8_t* data;
                size_t size;
                std::function<std::unique_ptr<musac::decoder>()> create_decoder;
                const char* name;
            };
            
            TestCase tests[] = {
                { test16_aiff_input, test16_aiff_input_size, 
                  []() { return std::make_unique<musac::decoder_aiff>(); }, "AIFF" },
                { file_1_voc_input, file_1_voc_input_size,
                  []() { return std::make_unique<musac::decoder_voc>(); }, "VOC" },
                { soundcard_wav_input, soundcard_wav_input_size,
                  []() { return std::make_unique<musac::decoder_drwav>(); }, "WAV" }
            };
            
            for (const auto& test : tests) {
                INFO("Testing " << test.name << " decoder consistency");
                
                // Decode twice and compare
                std::vector<float> first_decode, second_decode;
                
                for (int i = 0; i < 2; ++i) {
                    auto io = musac::io_from_memory(test.data, test.size);
                    REQUIRE(io != nullptr);
                    
                    auto decoder = test.create_decoder();
                    REQUIRE(decoder->open(io.get()));
                    
                    auto& target = (i == 0) ? first_decode : second_decode;
                    target = decodeAll(*decoder, decoder->get_channels());
                }
                
                CHECK(first_decode.size() == second_decode.size());
                CHECK(compareFloatArrays(first_decode.data(), second_decode.data(), 
                                       first_decode.size()));
            }
        }
    }
}