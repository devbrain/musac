#include <doctest/doctest.h>
#include <musac/codecs/decoder_voc.hh>
#include <musac/sdk/io_stream.hh>
#include <vector>
#include <cmath>
#include <string>

// Include all VOC format golden data
#include "golden_data/voc_all_formats.h"

// Helper function to compare float arrays with tolerance
bool compareFloatArrays(const float* expected, const float* actual, size_t size, float tolerance = 0.01f) {
    for (size_t i = 0; i < size; ++i) {
        if (std::fabs(expected[i] - actual[i]) > tolerance) {
            // Report first mismatch for debugging
            MESSAGE("First mismatch at index ", i, ": expected ", expected[i], ", got ", actual[i]);
            return false;
        }
    }
    return true;
}

// Helper function to decode entire stream
std::vector<float> decodeAll(musac::decoder& decoder, size_t channels) {
    std::vector<float> output;
    const size_t buffer_size = 4096;
    float buffer[buffer_size];
    bool more_data = true;
    
    while (more_data) {
        size_t decoded = decoder.decode(buffer, buffer_size, more_data, static_cast<musac::channels_t>(channels));
        if (decoded > 0) {
            output.insert(output.end(), buffer, buffer + decoded);
        }
    }
    
    return output;
}

TEST_SUITE("VOC Decoder Golden Data") {
    TEST_CASE("All VOC Formats") {
        // Test each VOC format
        for (const auto & test : voc_golden_tests) {
            SUBCASE(test.name) {
                INFO("Testing VOC format: ", test.name);
                INFO("Sample rate: ", test.sample_rate, " Hz");
                INFO("Channels: ", test.channels);
                
                // Create IO stream from input data
                auto io = musac::io_from_memory(test.input, test.input_size);
                REQUIRE(io != nullptr);
                
                musac::decoder_voc decoder;
                
                // Test opening the file
                CHECK_NOTHROW(decoder.open(io.get()));
                CHECK(decoder.is_open());
                CHECK(decoder.get_channels() == test.channels);
                CHECK(decoder.get_rate() == test.sample_rate);
                
                // Test format detection
                io->seek(0, musac::seek_origin::set);
                CHECK(decoder.accept(io.get()));
                
                // Decode and validate output
                io->seek(0, musac::seek_origin::set);
                decoder.open(io.get());
                
                auto decoded = decodeAll(decoder, test.channels);
                
                // For ADPCM formats, allow more tolerance due to lossy compression
                float tolerance = 0.01f;
                if (std::string(test.name).find("ADPCM") != std::string::npos) {
                    tolerance = 0.1f;  // ADPCM is lossy, allow more tolerance
                }
                
                // Check size matches (or is close for truncated data)
                if (!test.truncated) {
                    CHECK(decoded.size() == test.expected_output_size);
                } else {
                    // For truncated data, we should have at least the truncated amount
                    CHECK(decoded.size() >= test.expected_output_size);
                }
                
                // Compare output with expected (ffmpeg reference)
                size_t samples_to_compare = std::min(decoded.size(), test.expected_output_size);
                if (samples_to_compare > 0) {
                    CHECK(compareFloatArrays(test.expected_output, decoded.data(), 
                                           samples_to_compare, tolerance));
                }
                
                // Test rewind functionality
                CHECK_NOTHROW(decoder.rewind());
                auto decoded_after_rewind = decodeAll(decoder, test.channels);
                CHECK(decoded_after_rewind.size() == decoded.size());
            }
        }
    }
}