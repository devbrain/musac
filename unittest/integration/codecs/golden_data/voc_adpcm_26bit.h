// Generated golden data for voc_adpcm_26bit
#pragma once

#include <cstdint>
#include <cstddef>

// Input VOC file data (47 bytes)
const uint8_t voc_adpcm_26bit_input[] = {
    0x43, 0x72, 0x65, 0x61, 0x74, 0x69, 0x76, 0x65, 0x20, 0x56, 0x6f, 0x69, 0x63, 0x65, 0x20, 0x46, 
    0x69, 0x6c, 0x65, 0x1a, 0x1a, 0x00, 0x0a, 0x01, 0x29, 0x11, 0x01, 0x0e, 0x00, 0x00, 0x7b, 0x02, 
    0x80, 0x07, 0x00, 0x00, 0x00, 0x01, 0xa1, 0xa0, 0x00, 0x35, 0x03, 0x00, 0x03, 0x03, 0x00
};
const size_t voc_adpcm_26bit_input_size = 47;

// Expected output (36 samples)
const float voc_adpcm_26bit_output[] = {
    7.812500e-03f, -1.562500e-02f, -2.343750e-02f, -4.687500e-02f, -6.250000e-02f, -4.687500e-02f, -7.031250e-02f, -9.375000e-02f, 
    -1.015625e-01f, -1.250000e-01f, -1.484375e-01f, -1.562500e-01f, -1.796875e-01f, -2.031250e-01f, -2.109375e-01f, -2.343750e-01f, 
    -2.578125e-01f, -2.578125e-01f, -2.421875e-01f, -2.656250e-01f, -2.656250e-01f, -2.500000e-01f, -2.734375e-01f, -2.812500e-01f, 
    -3.046875e-01f, -3.281250e-01f, -3.359375e-01f, -3.515625e-01f, -3.359375e-01f, -3.359375e-01f, -3.593750e-01f, -3.828125e-01f, 
    -3.671875e-01f, -3.906250e-01f, -4.140625e-01f, -4.218750e-01f
};
const size_t voc_adpcm_26bit_output_size = 36;

// Metadata
const unsigned int voc_adpcm_26bit_rate = 7518;
const unsigned int voc_adpcm_26bit_channels = 1;
const bool voc_adpcm_26bit_truncated = false;
