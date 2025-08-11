/*
 * fmdrv - very accurate C port of SBFMDRV CMF replayer.
 *
 * Copyright 2024 Sergei "x0r" Kolzun
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CMF_H
#define CMF_H

#include <stdint.h>
#include <string.h>  /* for memcpy */

/* Read 16-bit little-endian value from byte array */
/* Endianness is detected by CMake and passed as MUSAC_LITTLE_ENDIAN or MUSAC_BIG_ENDIAN */
static inline uint16_t read_16le(const uint8_t *p) {
#ifdef MUSAC_LITTLE_ENDIAN
    /* On little-endian hosts, we can read directly */
    /* Use memcpy to avoid alignment issues and undefined behavior */
    uint16_t value;
    memcpy(&value, p, sizeof(value));
    return value;
#elif defined(MUSAC_BIG_ENDIAN)
    /* On big-endian hosts, need to swap bytes */
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
#else
    /* Fallback: neither endianness defined - use safe portable method */
    /* This ensures byte-by-byte reading in little-endian order */
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
#endif
}

/* Macro wrapper for backward compatibility */
#define READ_16LE(p) read_16le((const uint8_t *)(p))

#if defined(__cplusplus)
extern "C" {
#endif

struct fmdrv_s;

uint16_t sbfm_version(void);
uint8_t *sbfm_status_addx(struct fmdrv_s* obj, uint8_t *new_val);
void sbfm_instrument(struct fmdrv_s* obj, uint8_t *inst_table, int num_inst);
void sbfm_song_speed(struct fmdrv_s* obj, uint16_t fdiv);
void sbfm_play_music(struct fmdrv_s* obj, uint8_t *cmf_music_blk);
void sbfm_pause_music(struct fmdrv_s* obj);
void sbfm_resume_music(struct fmdrv_s* obj);


struct fmdrv_s* sbfm_init(size_t srate);
int sbfm_get_rate(struct fmdrv_s* obj);
void sbfm_destroy(struct fmdrv_s* obj);

void sbfm_tick(struct fmdrv_s* obj);
//void sbfm_render(struct fmdrv_s* obj, int16_t* sample_pairs, int count);
//int16_t sbfm_render(struct fmdrv_s* obj);
void sbfm_render_stereo(struct fmdrv_s* obj, float* a, float* b);
void sbfm_reset(struct fmdrv_s* obj);
int sbfm_get_chip_rate(struct fmdrv_s* obj);

// Fast-forward support for duration and seeking
uint32_t sbfm_calculate_duration_samples(struct fmdrv_s* obj);
int sbfm_seek_to_sample(struct fmdrv_s* obj, uint32_t sample_pos);

#if defined(__cplusplus)
    }
#endif
#endif