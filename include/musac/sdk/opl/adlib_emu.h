//
// Created by igor on 3/23/25.
//

#ifndef ADLIB_EMU_H
#define ADLIB_EMU_H

#include <stdint.h>

#include <musac/sdk/export_musac_sdk.h>

#if defined(__cplusplus)
extern "C" {
#endif

// initialize the chip(s). samplerate is typicallly 44100.
MUSAC_SDK_EXPORT  void* adlib_init(unsigned samplerate);
MUSAC_SDK_EXPORT  void  adlib_destroy(void* sb);

// write a value into a register.
MUSAC_SDK_EXPORT  void adlib_write_data(void* sb, uint16_t reg, uint8_t val);
MUSAC_SDK_EXPORT  void adlib_get_sample_stereo(void* sb, float* first, float* second);
MUSAC_SDK_EXPORT  float adlib_get_sample(void* sb);

#if defined(__cplusplus)
}
#endif

#endif
