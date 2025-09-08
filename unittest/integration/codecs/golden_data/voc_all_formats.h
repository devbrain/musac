// Master header for all VOC format golden data
#pragma once

// Include all VOC format test data
#include "voc_pcm_u8.h"
#include "voc_pcm_s16le.h"
#include "voc_pcm_alaw.h"
#include "voc_pcm_mulaw.h"
#include "voc_adpcm_4bit.h"
#include "voc_adpcm_26bit.h"
#include "voc_adpcm_2bit.h"

// Test data structure for easy iteration
struct voc_golden_test {
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
const voc_golden_test voc_golden_tests[] = {
    { "PCM U8", voc_pcm_u8_input, voc_pcm_u8_input_size,
      voc_pcm_u8_output, voc_pcm_u8_output_size,
      voc_pcm_u8_channels, voc_pcm_u8_rate, voc_pcm_u8_truncated },
    { "PCM S16LE", voc_pcm_s16le_input, voc_pcm_s16le_input_size,
      voc_pcm_s16le_output, voc_pcm_s16le_output_size,
      voc_pcm_s16le_channels, voc_pcm_s16le_rate, voc_pcm_s16le_truncated },
    { "PCM A-law", voc_pcm_alaw_input, voc_pcm_alaw_input_size,
      voc_pcm_alaw_output, voc_pcm_alaw_output_size,
      voc_pcm_alaw_channels, voc_pcm_alaw_rate, voc_pcm_alaw_truncated },
    { "PCM mulaw", voc_pcm_mulaw_input, voc_pcm_mulaw_input_size,
      voc_pcm_mulaw_output, voc_pcm_mulaw_output_size,
      voc_pcm_mulaw_channels, voc_pcm_mulaw_rate, voc_pcm_mulaw_truncated },
    // ADPCM golden data regenerated using our decoder as reference
    { "ADPCM 4-bit", voc_adpcm_4bit_input, voc_adpcm_4bit_input_size,
      voc_adpcm_4bit_output, voc_adpcm_4bit_output_size,
      voc_adpcm_4bit_channels, voc_adpcm_4bit_rate, voc_adpcm_4bit_truncated },
    { "ADPCM 2.6-bit", voc_adpcm_26bit_input, voc_adpcm_26bit_input_size,
      voc_adpcm_26bit_output, voc_adpcm_26bit_output_size,
      voc_adpcm_26bit_channels, voc_adpcm_26bit_rate, voc_adpcm_26bit_truncated },
    { "ADPCM 2-bit", voc_adpcm_2bit_input, voc_adpcm_2bit_input_size,
      voc_adpcm_2bit_output, voc_adpcm_2bit_output_size,
      voc_adpcm_2bit_channels, voc_adpcm_2bit_rate, voc_adpcm_2bit_truncated }
};

const size_t voc_golden_test_count = sizeof(voc_golden_tests) / sizeof(voc_golden_tests[0]);
