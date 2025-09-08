// Master header for all AIFF format golden data
#pragma once

#if defined(__clang__) || defined(__GNUC__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

// Include all AIFF format test data
#include "aiff_M1F1_int16_AFsp.h"
#include "aiff_M1F1_int8_AFsp.h"
#include "aiff_M1F1_int32C_AFsp.h"
#include "aiff_M1F1_int12_AFsp.h"
#include "aiff_M1F1_float32C_AFsp.h"
#include "aiff_M1F1_float64C_AFsp.h"
#include "aiff_M1F1_AlawC_AFsp.h"
#include "aiff_ulaw.h"
#include "aiff_alaw.h"
#include "aiff_PondSong.h"

#if defined(__clang__) || defined(__GNUC__)
        #pragma GCC diagnostic pop
#endif

//
// Test data structure for easy iteration
struct aiff_golden_test {
    const char* name;
    const uint8_t* input;
    size_t input_size;
    const float* expected_output;
    size_t expected_output_size;
    unsigned int channels;
    unsigned int sample_rate;
    bool truncated;
};

// Array of all test cases
const aiff_golden_test aiff_golden_tests[] = {
    { "PCM 16-bit BE", aiff_M1F1_int16_AFsp_input, aiff_M1F1_int16_AFsp_input_len,
      aiff_M1F1_int16_AFsp_output, aiff_M1F1_int16_AFsp_output_size,
      aiff_M1F1_int16_AFsp_channels, aiff_M1F1_int16_AFsp_rate, aiff_M1F1_int16_AFsp_truncated },
      
    { "PCM 8-bit", aiff_M1F1_int8_AFsp_input, aiff_M1F1_int8_AFsp_input_len,
      aiff_M1F1_int8_AFsp_output, aiff_M1F1_int8_AFsp_output_size,
      aiff_M1F1_int8_AFsp_channels, aiff_M1F1_int8_AFsp_rate, aiff_M1F1_int8_AFsp_truncated },
      
    { "PCM 32-bit BE", aiff_M1F1_int32C_AFsp_input, aiff_M1F1_int32C_AFsp_input_len,
      aiff_M1F1_int32C_AFsp_output, aiff_M1F1_int32C_AFsp_output_size,
      aiff_M1F1_int32C_AFsp_channels, aiff_M1F1_int32C_AFsp_rate, aiff_M1F1_int32C_AFsp_truncated },
      
    { "PCM 12-bit", aiff_M1F1_int12_AFsp_input, aiff_M1F1_int12_AFsp_input_len,
      aiff_M1F1_int12_AFsp_output, aiff_M1F1_int12_AFsp_output_size,
      aiff_M1F1_int12_AFsp_channels, aiff_M1F1_int12_AFsp_rate, aiff_M1F1_int12_AFsp_truncated },
      
    { "Float 32-bit BE", aiff_M1F1_float32C_AFsp_input, aiff_M1F1_float32C_AFsp_input_len,
      aiff_M1F1_float32C_AFsp_output, aiff_M1F1_float32C_AFsp_output_size,
      aiff_M1F1_float32C_AFsp_channels, aiff_M1F1_float32C_AFsp_rate, aiff_M1F1_float32C_AFsp_truncated },
      
    { "Float 64-bit BE", aiff_M1F1_float64C_AFsp_input, aiff_M1F1_float64C_AFsp_input_len,
      aiff_M1F1_float64C_AFsp_output, aiff_M1F1_float64C_AFsp_output_size,
      aiff_M1F1_float64C_AFsp_channels, aiff_M1F1_float64C_AFsp_rate, aiff_M1F1_float64C_AFsp_truncated },
      
    { "A-law", aiff_M1F1_AlawC_AFsp_input, aiff_M1F1_AlawC_AFsp_input_len,
      aiff_M1F1_AlawC_AFsp_output, aiff_M1F1_AlawC_AFsp_output_size,
      aiff_M1F1_AlawC_AFsp_channels, aiff_M1F1_AlawC_AFsp_rate, aiff_M1F1_AlawC_AFsp_truncated },
      
    { "µ-law", aiff_ulaw_input, aiff_ulaw_input_len,
      aiff_ulaw_output, aiff_ulaw_output_size,
      aiff_ulaw_channels, aiff_ulaw_rate, aiff_ulaw_truncated },
      
    { "A-law (AIFC)", aiff_alaw_input, aiff_alaw_input_len,
      aiff_alaw_output, aiff_alaw_output_size,
      aiff_alaw_channels, aiff_alaw_rate, aiff_alaw_truncated },
      
    { "IMA ADPCM", aiff_PondSong_input, aiff_PondSong_input_len,
      aiff_PondSong_output, aiff_PondSong_output_size,
      aiff_PondSong_channels, aiff_PondSong_rate, aiff_PondSong_truncated }
};

const size_t aiff_golden_test_count = sizeof(aiff_golden_tests) / sizeof(aiff_golden_tests[0]);