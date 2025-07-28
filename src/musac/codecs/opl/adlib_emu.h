//
// Created by igor on 3/23/25.
//

#ifndef ADLIB_EMU_H
#define ADLIB_EMU_H

#include <stdint.h>

#if defined(__cplusplus)
#define ADLIB_EMU_EXTERN_C extern "C"
#else
#define ADLIB_EMU_EXTERN_C
#endif

// initialize the chip(s). samplerate is typicallly 44100.
ADLIB_EMU_EXTERN_C void* adlib_init(unsigned samplerate);
ADLIB_EMU_EXTERN_C void  adlib_destroy(void* sb);

// write a value into a register.
ADLIB_EMU_EXTERN_C void adlib_write_data(void* sb, uint16_t reg, uint8_t val);
ADLIB_EMU_EXTERN_C void adlib_get_sample_stereo(void* sb, float* first, float* second);
ADLIB_EMU_EXTERN_C float adlib_get_sample(void* sb);

#endif
