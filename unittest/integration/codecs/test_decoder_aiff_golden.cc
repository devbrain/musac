#include <doctest/doctest.h>
#include <musac/codecs/decoder_aiff.hh>
#include <musac/sdk/io_stream.hh>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <numeric>

// Include all AIFF format golden data
#include "golden_data/aiff_all_formats.h"

// Helper function to calculate RMS (Root Mean Square) error
double calculateRMS(const float* expected, const float* actual, size_t size) {
    if (size == 0) return 0.0;
    
    double sum_squared_diff = 0.0;
    for (size_t i = 0; i < size; ++i) {
        auto diff = expected[i] - actual[i];
        sum_squared_diff += static_cast<double>(diff) * static_cast<double>(diff);
    }
    
    return std::sqrt(sum_squared_diff / static_cast<double>(size));
}

// Helper function to calculate signal RMS (for SNR calculation)
double calculateSignalRMS(const float* signal, size_t size) {
    if (size == 0) return 0.0;
    
    double sum_squared = 0.0;
    for (size_t i = 0; i < size; ++i) {
        sum_squared += static_cast<double>(signal[i] * signal[i]);
    }
    
    return std::sqrt(sum_squared / static_cast<double>(size));
}

// Helper function to calculate SNR in dB
double calculateSNR(const float* expected, const float* actual, size_t size) {
    double signal_rms = calculateSignalRMS(expected, size);
    double error_rms = calculateRMS(expected, actual, size);
    
    if (error_rms < 1e-10) return 100.0; // Very high SNR for near-perfect match
    if (signal_rms < 1e-10) return 0.0;  // No signal
    
    return 20.0 * std::log10(signal_rms / error_rms);
}

// Helper function to compare float arrays with RMS tolerance
bool compareWithRMS(const float* expected, const float* actual, size_t size, 
                    double max_rms, double min_snr_db, const std::string& format_name) {
    double rms = calculateRMS(expected, actual, size);
    double snr = calculateSNR(expected, actual, size);
    
    MESSAGE("Format: ", format_name);
    MESSAGE("RMS Error: ", rms);
    MESSAGE("SNR: ", snr, " dB");
    
    // Find maximum absolute difference for additional info
    double max_diff = 0.0;
    size_t max_diff_idx = 0;
    for (size_t i = 0; i < size; ++i) {
        double diff = std::abs(expected[i] - actual[i]);
        if (diff > max_diff) {
            max_diff = diff;
            max_diff_idx = i;
        }
    }
    MESSAGE("Max absolute difference: ", max_diff, " at sample ", max_diff_idx);
    
    // Check both RMS and SNR criteria
    bool rms_ok = rms <= max_rms;
    bool snr_ok = snr >= min_snr_db;
    
    if (!rms_ok) {
        MESSAGE("RMS error ", rms, " exceeds threshold ", max_rms);
    }
    if (!snr_ok) {
        MESSAGE("SNR ", snr, " dB is below minimum ", min_snr_db, " dB");
    }
    
    return rms_ok && snr_ok;
}

// Helper function to decode entire stream
static std::vector<float> decodeAll(musac::decoder& decoder, size_t channels) {
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


TEST_SUITE("AIFF Decoder Golden Data") {
    TEST_CASE("All AIFF Formats with RMS Validation") {
        // Test each AIFF format
        for (const auto& test : aiff_golden_tests) {
            SUBCASE(test.name) {
                INFO("Testing AIFF format: ", std::string(test.name));
                INFO("Sample rate: ", test.sample_rate, " Hz");
                INFO("Channels: ", test.channels);
                const std::string t = test.truncated ? "yes" : "no";
                INFO("Truncated: ", t);
                
                // Create IO stream from input data
                auto io = musac::io_from_memory(test.input, test.input_size);
                REQUIRE(io != nullptr);
                
                musac::decoder_aiff decoder;
                
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
                
                // Set tolerance based on format
                double max_rms = 0.001;  // Default RMS tolerance
                double min_snr_db = 40.0; // Default minimum SNR in dB
                
                // Adjust tolerance for different formats
                if (std::string(test.name).find("8-bit") != std::string::npos) {
                    max_rms = 0.004;  // 8-bit has lower precision
                    min_snr_db = 35.0;
                } else if (std::string(test.name).find("12-bit") != std::string::npos) {
                    max_rms = 0.001;  // 12-bit moderate precision
                    min_snr_db = 38.0;
                } else if (std::string(test.name).find("law") != std::string::npos) {
                    max_rms = 0.01;   // µ-law/A-law are lossy
                    min_snr_db = 30.0;
                } else if (std::string(test.name).find("ADPCM") != std::string::npos ||
                          std::string(test.name).find("IMA") != std::string::npos) {
                    max_rms = 0.02;   // ADPCM is lossy compression
                    min_snr_db = 25.0;
                } else if (std::string(test.name).find("Float") != std::string::npos) {
                    max_rms = 0.0001; // Float formats should be very accurate
                    min_snr_db = 50.0;
                }
                
                // Check size matches (or is close for truncated data)
                if (!test.truncated) {
                    MESSAGE("Decoded size: ", decoded.size(), ", Expected size: ", test.expected_output_size);
                    CHECK(decoded.size() == test.expected_output_size);
                } else {
                    // For truncated data, we should have approximately the expected amount
                    // Allow 1% tolerance for rounding differences
                    double size_ratio = static_cast<double>(decoded.size()) / static_cast<double>(test.expected_output_size);
                    MESSAGE("Decoded size: ", decoded.size(), ", Expected size: ", test.expected_output_size, ", Ratio: ", size_ratio);
                    CHECK(size_ratio >= 0.99);
                    CHECK(size_ratio <= 1.01);
                }
                
                // Compare output with expected using RMS metric
                size_t samples_to_compare = std::min(decoded.size(), test.expected_output_size);
                if (samples_to_compare > 0) {
                    CHECK(compareWithRMS(test.expected_output, decoded.data(), 
                                        samples_to_compare, max_rms, min_snr_db, test.name));
                }
                
                // Test rewind functionality
                CHECK_NOTHROW(decoder.rewind());
                auto decoded_after_rewind = decodeAll(decoder, test.channels);
                CHECK(decoded_after_rewind.size() == decoded.size());
                
                // Verify rewind produces identical output
                if (decoded.size() == decoded_after_rewind.size() && !decoded.empty()) {
                    double rewind_rms = calculateRMS(decoded.data(), decoded_after_rewind.data(), decoded.size());
                    CHECK(rewind_rms < 1e-10); // Should be identical after rewind
                }
            }
        }
    }
    
    TEST_CASE("AIFF Format Detection") {
        for (const auto& test : aiff_golden_tests) {
            SUBCASE(test.name) {
                INFO("Testing format detection for: ", test.name);
                
                auto io = musac::io_from_memory(test.input, test.input_size);
                REQUIRE(io != nullptr);
                
                // Static accept method should work
                CHECK(musac::decoder_aiff::accept(io.get()));
                CHECK(io->tell() == 0); // Position should be restored
                
                // Instance accept should also work
                musac::decoder_aiff decoder;
                CHECK(decoder.accept(io.get()));
                CHECK(io->tell() == 0); // Position should be restored
                
                // Should still be able to open after accept
                CHECK_NOTHROW(decoder.open(io.get()));
                CHECK(decoder.is_open());
            }
        }
    }
    
    TEST_CASE("AIFF Seek and Duration") {
        for (const auto& test : aiff_golden_tests) {
            // Skip very short files for seek tests
            if (test.expected_output_size < 1000) continue;
            
            SUBCASE(test.name) {
                INFO("Testing seek/duration for: ", test.name);
                
                auto io = musac::io_from_memory(test.input, test.input_size);
                REQUIRE(io != nullptr);
                
                musac::decoder_aiff decoder;
                REQUIRE_NOTHROW(decoder.open(io.get()));
                
                // Get duration
                auto duration = decoder.duration();
                if (duration.count() > 0) {
                    INFO("Duration: ", static_cast<float>(duration.count()) / 1000000.0f, " seconds");
                    
                    // Try seeking to middle
                    auto middle = duration / 2;
                    bool seek_result = decoder.seek_to_time(middle);
                    
                    if (seek_result) {
                        // Decode some samples from middle
                        float buffer[1024];
                        bool more_data;
                        size_t decoded = decoder.decode(buffer, 1024, more_data, static_cast<musac::channels_t>(test.channels));
                        CHECK(decoded > 0);
                        
                        // Seek back to beginning
                        CHECK(decoder.seek_to_time(std::chrono::microseconds(0)));
                        
                        // Verify we're back at the beginning
                        float begin_buffer[1024];
                        decoded = decoder.decode(begin_buffer, 1024, more_data, static_cast<musac::channels_t>(test.channels));
                        CHECK(decoded > 0);
                        
                        // First samples should match expected output
                        if (decoded >= 10) {
                            double begin_rms = calculateRMS(test.expected_output, begin_buffer, 
                                                           std::min(size_t(10), decoded));
                            CHECK(begin_rms < 0.1); // Reasonable tolerance for start
                        }
                    }
                }
            }
        }
    }
}