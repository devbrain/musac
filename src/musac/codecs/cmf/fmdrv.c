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

#if defined(_MSC_VER)
#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
#pragma warning(disable : 4242 4244 4018 4267 4389 4100)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// #include "opl.c"

#include "codecs/opl/adlib_emu.h"

#include "fmdrv.h"

typedef struct opl_voice_s {
    uint8_t midi_chn;
    uint8_t block_note;
    uint8_t midi_note;
    uint8_t KSL;
    uint8_t level;
    uint16_t fnum;
    uint16_t start;
} opl_voice_t;

typedef struct midi_channel_s {
    uint8_t inst;
    int16_t transp;
    uint8_t mute;
} midi_channel_t;

typedef struct fmdrv_s {
    size_t g_srate;
    size_t g_song_timer_fdiv, g_song_timer_rate, g_song_timer_cnt;
    size_t g_sys_timer_fdiv, g_sys_timer_cnt;
    int g_num_inst, g_opl_chan_num;
    uint8_t g_opl_perc_mode, g_opl_BD;
    uint8_t g_midi_cmd, g_midi_chn;
    uint8_t ct_music_status,* pct_music_status;
    uint8_t* g_inst_table;
    uint8_t* g_music_blk,* g_music_pos;
    uint8_t g_status;
    int16_t g_transp;
    uint16_t g_events;
    uint32_t g_delay;

    opl_voice_t g_opl_voices[11];
    midi_channel_t g_midi_channels[16];

    void* m_chip;
} fmdrv_t;

static uint8_t opl_reg_offs[9] = {0x00, 0x01, 0x02, 0x08, 0x09, 0x0a, 0x10, 0x11, 0x12};
static uint8_t init_inst[11] = {0x01, 0x11, 0x4F, 0x00, 0xf1, 0xf2, 0x53, 0x74, 0x00, 0x00, 0x08};
static uint8_t opl_perc_offs[5] = {0x10, 0x14, 0x12, 0x15, 0x11};
static uint8_t opl_perc_mask[5] = {16, 8, 4, 2, 1};
static uint8_t opl_perc_voice[5] = {6, 7, 8, 8, 7};

#define adlib_write(a, b) adlib_write_data(obj->m_chip, a ,b);

static uint8_t default_inst_bank[] = {
    0x21, 0x21, 0xd1, 0x07, 0xa3, 0xa4, 0x46, 0x25, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x22, 0x22, 0x0f, 0x0f, 0xf6, 0xf6, 0x95, 0x36, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xe1, 0xe1, 0x00, 0x00, 0x44, 0x54, 0x24, 0x34, 0x02, 0x02, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xa5, 0xb1, 0xd2, 0x80, 0x81, 0xf1, 0x03, 0x05, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x71, 0x22, 0xc5, 0x05, 0x6e, 0x8b, 0x17, 0x0e, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x32, 0x21, 0x16, 0x80, 0x73, 0x75, 0x24, 0x57, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x01, 0x11, 0x4f, 0x00, 0xf1, 0xd2, 0x53, 0x74, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x07, 0x12, 0x4f, 0x00, 0xf2, 0xf2, 0x60, 0x72, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x31, 0xa1, 0x1c, 0x80, 0x51, 0x54, 0x03, 0x67, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x31, 0xa1, 0x1c, 0x80, 0x41, 0x92, 0x0b, 0x3b, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x31, 0x16, 0x87, 0x80, 0xa1, 0x7d, 0x11, 0x43, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x30, 0xb1, 0xc8, 0x80, 0xd5, 0x61, 0x19, 0x1b, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x04,
    0xf1, 0x21, 0x01, 0x0d, 0x97, 0xf1, 0x17, 0x18, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x32, 0x16, 0x87, 0x80, 0xa1, 0x7d, 0x10, 0x33, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x01, 0x12, 0x4f, 0x00, 0x71, 0x52, 0x53, 0x7c, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x02, 0x03, 0x8d, 0x03, 0xd7, 0xf5, 0x37, 0x18, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint8_t block_note_tbl[128] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b,
    0x7b, 0x7b, 0x7b, 0x7b, 0x7b, 0x7b, 0x7b, 0x7b
};

static uint16_t fnum_tbl[768] = {
    343, 343, 344, 344, 344, 344, 345, 345, 345, 346, 346, 346,
    347, 347, 347, 348, 348, 348, 349, 349, 349, 349, 350, 350,
    350, 351, 351, 351, 352, 352, 352, 353, 353, 353, 354, 354,
    354, 355, 355, 355, 356, 356, 356, 356, 357, 357, 357, 358,
    358, 358, 359, 359, 359, 360, 360, 360, 361, 361, 361, 362,
    362, 362, 363, 363, 363, 364, 364, 364, 365, 365, 365, 366,
    366, 366, 367, 367, 367, 368, 368, 368, 369, 369, 369, 370,
    370, 370, 371, 371, 371, 372, 372, 372, 373, 373, 373, 374,
    374, 374, 375, 375, 375, 376, 376, 376, 377, 377, 377, 378,
    378, 378, 379, 379, 379, 380, 380, 380, 381, 381, 381, 382,
    382, 382, 383, 383, 384, 384, 384, 385, 385, 385, 386, 386,
    386, 387, 387, 387, 388, 388, 388, 389, 389, 389, 390, 390,
    391, 391, 391, 392, 392, 392, 393, 393, 393, 394, 394, 394,
    395, 395, 395, 396, 396, 397, 397, 397, 398, 398, 398, 399,
    399, 399, 400, 400, 401, 401, 401, 402, 402, 402, 403, 403,
    403, 404, 404, 405, 405, 405, 406, 406, 406, 407, 407, 407,
    408, 408, 409, 409, 409, 410, 410, 410, 411, 411, 412, 412,
    412, 413, 413, 413, 414, 414, 414, 415, 415, 416, 416, 416,
    417, 417, 417, 418, 418, 419, 419, 419, 420, 420, 421, 421,
    421, 422, 422, 422, 423, 423, 424, 424, 424, 425, 425, 425,
    426, 426, 427, 427, 427, 428, 428, 429, 429, 429, 430, 430,
    430, 431, 431, 432, 432, 432, 433, 433, 434, 434, 434, 435,
    435, 436, 436, 436, 437, 437, 438, 438, 438, 439, 439, 440,
    440, 440, 441, 441, 442, 442, 442, 443, 443, 444, 444, 444,
    445, 445, 446, 446, 446, 447, 447, 448, 448, 448, 449, 449,
    450, 450, 450, 451, 451, 452, 452, 452, 453, 453, 454, 454,
    454, 455, 455, 456, 456, 457, 457, 457, 458, 458, 459, 459,
    459, 460, 460, 461, 461, 461, 462, 462, 463, 463, 464, 464,
    464, 465, 465, 466, 466, 467, 467, 467, 468, 468, 469, 469,
    469, 470, 470, 471, 471, 472, 472, 472, 473, 473, 474, 474,
    475, 475, 475, 476, 476, 477, 477, 478, 478, 478, 479, 479,
    480, 480, 481, 481, 481, 482, 482, 483, 483, 484, 484, 485,
    485, 485, 486, 486, 487, 487, 488, 488, 488, 489, 489, 490,
    490, 491, 491, 492, 492, 492, 493, 493, 494, 494, 495, 495,
    496, 496, 496, 497, 497, 498, 498, 499, 499, 500, 500, 501,
    501, 501, 502, 502, 503, 503, 504, 504, 505, 505, 506, 506,
    506, 507, 507, 508, 508, 509, 509, 510, 510, 511, 511, 511,
    512, 512, 513, 513, 514, 514, 515, 515, 516, 516, 517, 517,
    518, 518, 518, 519, 519, 520, 520, 521, 521, 522, 522, 523,
    523, 524, 524, 525, 525, 526, 526, 526, 527, 527, 528, 528,
    529, 529, 530, 530, 531, 531, 532, 532, 533, 533, 534, 534,
    535, 535, 536, 536, 537, 537, 538, 538, 538, 539, 539, 540,
    540, 541, 541, 542, 542, 543, 543, 544, 544, 545, 545, 546,
    546, 547, 547, 548, 548, 549, 549, 550, 550, 551, 551, 552,
    552, 553, 553, 554, 554, 555, 555, 556, 556, 557, 557, 558,
    558, 559, 559, 560, 560, 561, 561, 562, 562, 563, 563, 564,
    564, 565, 565, 566, 566, 567, 567, 568, 568, 569, 569, 570,
    571, 571, 572, 572, 573, 573, 574, 574, 575, 575, 576, 576,
    577, 577, 578, 578, 579, 579, 580, 580, 581, 581, 582, 582,
    583, 584, 584, 585, 585, 586, 586, 587, 587, 588, 588, 589,
    589, 590, 590, 591, 591, 592, 593, 593, 594, 594, 595, 595,
    596, 596, 597, 597, 598, 598, 599, 600, 600, 601, 601, 602,
    602, 603, 603, 604, 604, 605, 606, 606, 607, 607, 608, 608,
    609, 609, 610, 610, 611, 612, 612, 613, 613, 614, 614, 615,
    615, 616, 617, 617, 618, 618, 619, 619, 620, 620, 621, 622,
    622, 623, 623, 624, 624, 625, 626, 626, 627, 627, 628, 628,
    629, 629, 630, 631, 631, 632, 632, 633, 633, 634, 635, 635,
    636, 636, 637, 637, 638, 639, 639, 640, 640, 641, 642, 642,
    643, 643, 644, 644, 645, 646, 646, 647, 647, 648, 649, 649,
    650, 650, 651, 651, 652, 653, 653, 654, 654, 655, 656, 656,
    657, 657, 658, 659, 659, 660, 660, 661, 662, 662, 663, 663,
    664, 665, 665, 666, 666, 667, 668, 668, 669, 669, 670, 671,
    671, 672, 672, 673, 674, 674, 675, 675, 676, 677, 677, 678,
    678, 679, 680, 680, 681, 682, 682, 683, 683, 684, 685, 685
};

static void note(struct fmdrv_s* obj);
static void note_off(struct fmdrv_s* obj, uint8_t note, uint8_t vel);
static void note_on(struct fmdrv_s* obj, uint8_t note, uint8_t vel);
static void process_controllers(struct fmdrv_s* obj);
static void prg_change(struct fmdrv_s* obj);
static void sysmsg(struct fmdrv_s* obj);
static void unimplemented(struct fmdrv_s* obj);
static void unimplemented1x(struct fmdrv_s* obj);
static void unimplemented2x(struct fmdrv_s* obj);

static void song_marker(struct fmdrv_s* obj, uint8_t v);
static void switch_mode(struct fmdrv_s* obj, uint8_t v);
static void transpose_up(struct fmdrv_s* obj, uint8_t v);
static void transpose_down(struct fmdrv_s* obj, uint8_t v);

static void sysmsg_sysex(struct fmdrv_s* obj);
static void sysmsg_eox(struct fmdrv_s* obj);
static void sysmsg_stop(struct fmdrv_s* obj);
static void sysmsg_reset(struct fmdrv_s* obj);

typedef void (* handler)(struct fmdrv_s*);
typedef void (* controller)(struct fmdrv_s*, uint8_t v);

static handler event_handlers[] = {
    note, // off
    note, // on
    unimplemented2x,
    process_controllers,
    prg_change,
    unimplemented1x,
    unimplemented2x,
    sysmsg,
};

static controller controller_handlers[] = {
    song_marker,
    switch_mode,
    transpose_up,
    transpose_down,
};

static handler sysmsg_handlers[] = {
    sysmsg_sysex,
    unimplemented,
    unimplemented2x,
    unimplemented1x,
    unimplemented,
    unimplemented,
    unimplemented,
    sysmsg_eox,
    unimplemented,
    unimplemented,
    unimplemented,
    unimplemented,
    sysmsg_stop,
    unimplemented,
    unimplemented,
    sysmsg_reset,
};

static uint32_t read_vlq(struct fmdrv_s* obj) {
    uint8_t b;
    uint32_t vlq = 0;

    do {
        vlq = (vlq << 7) | ((b = *obj->g_music_blk++) & 0x7f);
    }
    while (b & 0x80);

    return vlq;
}

static void opl_reset1(struct fmdrv_s* obj) {
    obj->g_opl_chan_num = 9;
    obj->g_opl_perc_mode = 0;
    obj->g_opl_BD = 0xc0;

    adlib_write(0xbd, obj->g_opl_BD);
}

static void opl_reset2(struct fmdrv_s* obj) {
    int i, j;
    uint8_t reg,* p_inst;

    for (i = 0; i < 16; ++i)
        obj->g_midi_channels[i].inst = 0;

    for (i = 0; i < 11; ++i)
        obj->g_opl_voices[i].midi_chn = 0xff;

    for (i = 0; i < 9; ++i) {
        adlib_write(0xbd, 0x00);
        adlib_write(0x08, 0x00);

        p_inst = init_inst;

        reg = opl_reg_offs[i];
        for (j = 0; j < 4; ++j) {
            reg += 0x20;
            adlib_write(reg, *p_inst++);
            reg += 3;
            adlib_write(reg, *p_inst++);
            reg -= 3;
        }

        reg += 0x60;
        adlib_write(reg, *p_inst++);
        reg += 3;
        adlib_write(reg, *p_inst++);

        reg = opl_reg_offs[i] + i; // BUG: should add 0xc0?
        adlib_write(reg, *p_inst++);
    }
}

static void midi_panic(struct fmdrv_s* obj) {
    uint8_t i;

    for (i = 0; i < obj->g_opl_chan_num; ++i) {
        adlib_write(0x83, 0x13); // BUG: should be (0x83 + i)?

        if (obj->g_opl_voices[i].midi_chn <= 0x7f) {
            adlib_write(0xa0 + i, obj->g_opl_voices[i].fnum & 0xff);
            adlib_write(0xb0 + i, obj->g_opl_voices[i].fnum >> 8);
        }
    }

    obj->g_opl_BD &= 0xe0;
    adlib_write(0xbd, obj->g_opl_BD);
}

static void set_timer(struct fmdrv_s* obj, uint16_t fdiv) {
    obj->g_song_timer_rate = obj->g_srate * fdiv / 1193180;
}

static void update_music_status(struct fmdrv_s* obj, uint8_t status) {
    *(obj->pct_music_status) = status;
}

static void stop_music(struct fmdrv_s* obj) {
    if (!obj->g_status)
        return;

    obj->g_status = 0;
    set_timer(obj, obj->g_sys_timer_fdiv);
    midi_panic(obj);
    update_music_status(obj, 0);
}

static void process_events(struct fmdrv_s* obj) {
    uint8_t event;

    ++obj->g_events;

    do {
        event = *obj->g_music_blk;
        if (event & 0x80) {
            obj->g_midi_chn = event & 0x0f;
            obj->g_midi_cmd = (event >> 4) - 8;
            ++obj->g_music_blk;
        }

        event_handlers[obj->g_midi_cmd](obj);

        if (!obj->g_status)
            return;
    }
    while ((obj->g_delay = read_vlq(obj)) == 0);

    --obj->g_delay;
}

static uint8_t find_opl_voice(struct fmdrv_s* obj, uint8_t midi_chn) {
    uint8_t i, m;
    uint16_t tmp, max = 0;

    for (i = 0; i < obj->g_opl_chan_num; ++i) {
        if (obj->g_opl_voices[i].midi_chn == (midi_chn | 0x80))
            return i;
    }

    for (i = 0; i < obj->g_opl_chan_num; ++i) {
        if (obj->g_opl_voices[i].midi_chn == 0xff)
            return i;
    }

    for (i = 0; i < obj->g_opl_chan_num; ++i) {
        if (obj->g_opl_voices[i].midi_chn > 0x7f)
            return i;
    }

    for (i = 0, m = 0; i < obj->g_opl_chan_num; ++i) {
        tmp = obj->g_events - obj->g_opl_voices[i].start;

        if (tmp > max) {
            max = tmp;
            m = i;
        }
    }

    adlib_write(0xa0 + m, obj->g_opl_voices[m].fnum & 0xff);
    adlib_write(0xb0 + m, obj->g_opl_voices[m].fnum >> 8);

    return m;
}

static uint16_t calc_block_fnum(struct fmdrv_s* obj, uint8_t n, uint8_t block_note) {
    int8_t block = (block_note & 0x70) >> 2;
    int16_t note = (block_note & 0x0f) << 6;
    uint16_t fnum;

    note += obj->g_midi_channels[obj->g_midi_chn].transp;

    if (note < 0) {
        note += 768;
        block -= 4;
        if (block < 0) {
            note = 0;
            block = 0;
        }
    }

    if (note >= 768) {
        note -= 768;
        block += 4;
        if (block > 28) {
            note = 767;
            block = 28;
        }
    }

    fnum = ((uint8_t)block << 8) | fnum_tbl[note];

    obj->g_opl_voices[n].fnum = fnum;
    obj->g_opl_voices[n].start = obj->g_events;

    return fnum;
}

static uint16_t note2fnum(struct fmdrv_s* obj, uint8_t n, uint8_t note) {
    int16_t note_transp;
    uint8_t block_note;

    obj->g_opl_voices[n].midi_note = note;

    note_transp = (int16_t)note + obj->g_transp;

    if (note_transp < 0)
        note_transp = 0;

    if (note_transp > 127)
        note_transp = 127;

    block_note = block_note_tbl[note_transp];
    obj->g_opl_voices[n].block_note = block_note;

    return calc_block_fnum(obj, n, block_note);
}

static void set_instrument(struct fmdrv_s* obj, uint8_t n, uint8_t insnum) {
    int i;
    uint8_t reg,* p_inst;

    if (insnum < obj->g_num_inst) {
        p_inst = &obj->g_inst_table[insnum << 4];

        obj->g_opl_voices[n].KSL = p_inst[3] & 0xc0;
        obj->g_opl_voices[n].level = 63 - (p_inst[3] & 0x3f);

        if (obj->g_opl_perc_mode == 0 || n <= 6) {
            reg = opl_reg_offs[n];
            for (i = 0; i < 4; ++i) {
                reg += 0x20;
                adlib_write(reg, *p_inst++);
                reg += 3;
                adlib_write(reg, *p_inst++);
                reg -= 3;
            }

            reg += 0x60;
            adlib_write(reg, *p_inst++);
            reg += 3;
            adlib_write(reg, *p_inst++);

            reg = 0xc0 + n;
            adlib_write(reg, *p_inst++);
        } else {
            reg = opl_perc_offs[n - 6];
            for (i = 0; i < 4; ++i) {
                reg += 0x20;
                adlib_write(reg, *p_inst++);
                ++p_inst;
            }

            reg += 0x60;
            adlib_write(reg, *p_inst++);
            ++p_inst;

            reg = 0xc0 + opl_perc_voice[n - 6];
            adlib_write(reg, (*p_inst++) | 1);
        }
    }
}

void note(struct fmdrv_s* obj) {
    uint8_t note = *obj->g_music_blk++;
    uint8_t vel = *obj->g_music_blk++;

    if (obj->g_midi_cmd == 0 || vel == 0)
        note_off(obj, note, vel);
    else
        note_on(obj, note, vel);
}

void note_off(struct fmdrv_s* obj, uint8_t note, uint8_t vel) {
    uint8_t i;

    if (obj->g_opl_chan_num > 6 || obj->g_midi_chn < 11) {
        for (i = 0; i < obj->g_opl_chan_num; ++i) {
            if (obj->g_opl_voices[i].midi_chn == obj->g_midi_chn && obj->g_opl_voices[i].midi_note == note) {
                obj->g_opl_voices[i].midi_chn |= 0x80;
                adlib_write(0xa0 + i, obj->g_opl_voices[i].fnum & 0xff);
                adlib_write(0xb0 + i, obj->g_opl_voices[i].fnum >> 8);
            }
        }
    } else {
        obj->g_opl_BD &= ~opl_perc_mask[obj->g_midi_chn - 5 - 6];
        adlib_write(0xbd, obj->g_opl_BD);
    }
}

void note_on(struct fmdrv_s* obj, uint8_t note, uint8_t vel) {
    uint16_t tmp;
    uint8_t opl_voice, midi_chn;

    if (!obj->g_midi_channels[obj->g_midi_chn].mute)
        return;

    if (obj->g_opl_perc_mode == 0 || obj->g_midi_chn < 11) {
        opl_voice = find_opl_voice(obj, obj->g_midi_chn);
        midi_chn = obj->g_opl_voices[opl_voice].midi_chn & 0x7f;
        obj->g_opl_voices[opl_voice].midi_chn = obj->g_midi_chn;

        if (obj->g_midi_chn != midi_chn) {
            set_instrument(obj, opl_voice, obj->g_midi_channels[obj->g_midi_chn].inst);
        }

        tmp = (vel | 0x80) * obj->g_opl_voices[opl_voice].level;
        vel = 63 - (tmp >> 8);
        vel |= obj->g_opl_voices[opl_voice].KSL;

        adlib_write(0x43 + opl_reg_offs[opl_voice], vel);

        tmp = note2fnum(obj, opl_voice, note);
        adlib_write(0xa0 + opl_voice, tmp & 0xff);
        adlib_write(0xb0 + opl_voice, 0x20 | (tmp >> 8));
    } else {
        opl_voice = obj->g_midi_chn - 5;
        obj->g_opl_BD |= opl_perc_mask[opl_voice - 6];

        tmp = (vel | 0x80) * obj->g_opl_voices[opl_voice].level;
        vel = 63 - (tmp >> 8);
        vel |= obj->g_opl_voices[opl_voice].KSL;

        tmp = 0x40;
        if (opl_voice == 6)
            tmp += 3;

        adlib_write(tmp + opl_perc_offs[opl_voice - 6], vel);

        tmp = note2fnum(obj, opl_voice, note);
        adlib_write(0xa0 + opl_voice, tmp & 0xff);
        adlib_write(0xb0 + opl_voice, (tmp >> 8));
        adlib_write(0xbd, obj->g_opl_BD);
    }
}

void process_controllers(struct fmdrv_s* obj) {
    uint8_t controller = (*obj->g_music_blk++) - 0x66;
    uint8_t value = *obj->g_music_blk++;

    if (controller < 4)
        controller_handlers[controller](obj, value);
}

void song_marker(struct fmdrv_s* obj, uint8_t v) {
    update_music_status(obj, v);
}

void switch_mode(struct fmdrv_s* obj, uint8_t v) {
    obj->g_opl_chan_num = 9;
    obj->g_opl_BD = 0xc0;
    obj->g_opl_perc_mode = v;

    if (obj->g_opl_perc_mode != 0) {
        obj->g_opl_chan_num = 6;
        obj->g_opl_BD = 0xe0;
    }

    obj->g_opl_voices[6].midi_chn = 0xff;
    obj->g_opl_voices[7].midi_chn = 0xff;
    obj->g_opl_voices[8].midi_chn = 0xff;

    adlib_write(0xbd, obj->g_opl_BD);
    opl_reset2(obj);
}

void transpose(struct fmdrv_s* obj, int8_t v) {
    uint8_t i;
    uint16_t fnum;

    obj->g_midi_channels[obj->g_midi_chn].transp = v / 4;

    for (i = 0; i < obj->g_opl_chan_num; ++i) {
        if (obj->g_opl_voices[i].midi_chn == obj->g_midi_chn) {
            fnum = calc_block_fnum(obj, i, obj->g_opl_voices[i].block_note);
            adlib_write(0xa0 + i, fnum & 0xff);
            adlib_write(0xb0 + i, 0x20 | (fnum >> 8));
        }
    }
}

void transpose_up(struct fmdrv_s* obj, uint8_t v) {
    transpose(obj, (int8_t)v);
}

void transpose_down(struct fmdrv_s* obj, uint8_t v) {
    transpose(obj, -(int8_t)v);
}

void prg_change(struct fmdrv_s* obj) {
    int8_t i;
    uint8_t insnum = *obj->g_music_blk++;

    insnum %= obj->g_num_inst;

    obj->g_midi_channels[obj->g_midi_chn].inst = insnum;

    if (obj->g_opl_perc_mode == 0 || obj->g_midi_chn < 11) {
        for (i = 0; i < obj->g_opl_chan_num; ++i) {
            if (obj->g_opl_voices[i].midi_chn == (obj->g_midi_chn | 0x80))
                obj->g_opl_voices[i].midi_chn = 0xff;
        }

        for (i = 0; i < obj->g_opl_chan_num; ++i) {
            if (obj->g_opl_voices[i].midi_chn == obj->g_midi_chn)
                set_instrument(obj, i, obj->g_midi_channels[obj->g_midi_chn].inst);
        }
    } else {
        set_instrument(obj, obj->g_midi_chn - 5, obj->g_midi_channels[obj->g_midi_chn].inst);
    }
}

void sysmsg(struct fmdrv_s* obj) {
    sysmsg_handlers[obj->g_midi_chn](obj);
}

void sysmsg_sysex(struct fmdrv_s* obj) {
    // call_trap
    obj->g_music_blk += read_vlq(obj);
}

void sysmsg_eox(struct fmdrv_s* obj) {
    obj->g_music_blk += read_vlq(obj);
}

void sysmsg_stop(struct fmdrv_s* obj) {
    stop_music(obj);
}

void sysmsg_reset(struct fmdrv_s* obj) {
    if (*obj->g_music_blk++ == 0x2f) {
        stop_music(obj);
    }

    obj->g_music_blk += read_vlq(obj);
}

void unimplemented(struct fmdrv_s* obj) {
}

void unimplemented1x(struct fmdrv_s* obj) {
    obj->g_music_blk++;
}

void unimplemented2x(struct fmdrv_s* obj) {
    obj->g_music_blk++;
    obj->g_music_blk++;
}

uint16_t sbfm_version(void) {
    return 0x010a;
}

uint8_t* sbfm_status_addx(struct fmdrv_s* obj, uint8_t* new) {
    uint8_t* old = obj->pct_music_status;

    obj->pct_music_status = new;
    update_music_status(obj, 0);

    return old;
}

void sbfm_instrument(struct fmdrv_s* obj, uint8_t* inst_table, int num_inst) {
    obj->g_num_inst = num_inst;
    obj->g_inst_table = inst_table;
    opl_reset1(obj);
    opl_reset2(obj);
}

void sbfm_song_speed(struct fmdrv_s* obj, uint16_t fdiv) {
    obj->g_song_timer_fdiv = fdiv;
    set_timer(obj, fdiv);
}

void sbfm_play_music(struct fmdrv_s* obj, uint8_t* cmf_music_blk) {
    int i;

    if (obj->g_status)
        return;

    obj->g_music_blk = obj->g_music_pos = cmf_music_blk;

    for (i = 0; i < 16; ++i)
        obj->g_midi_channels[i].transp = 0;

    for (i = 0; i < 9; ++i)
        obj->g_opl_voices[i].midi_chn = 0xff;

    obj->g_delay = read_vlq(obj);
    obj->g_events = 0;

    set_timer(obj, obj->g_song_timer_fdiv);
    obj->g_song_timer_cnt = 0;

    opl_reset1(obj);

    obj->g_status = 1;
    update_music_status(obj, 0xff);
}

void sbfm_pause_music(struct fmdrv_s* obj) {
    if (obj->g_status == 1) {
        obj->g_status = 2;
        midi_panic(obj);
    }
}

void sbfm_resume_music(struct fmdrv_s* obj) {
    if (obj->g_status == 2)
        obj->g_status = 1;
}

struct fmdrv_s* sbfm_init(size_t srate) {
    fmdrv_t* obj = malloc(sizeof(fmdrv_t));
    obj->m_chip = adlib_init(srate);
    //adlib_init(srate);
    //obj->m_chip = opl_create();
    // adlib_init(srate);
    obj->g_status = 0;
    obj->g_srate = srate;
    //timer_rate = 0xffff;
    obj->g_sys_timer_fdiv = 0xffff;
    obj->g_song_timer_fdiv = 18643;

    obj->ct_music_status = 0;
    obj->pct_music_status = &obj->ct_music_status;

    adlib_write(0x01, 0x20);
    adlib_write(0x08, 0x00);

    sbfm_reset(obj);

    return obj;
}

void sbfm_destroy(struct fmdrv_s* obj) {
    if (obj) {
        adlib_destroy(obj->m_chip);
        // opl_destroy(obj->m_chip);
        free(obj);
    }
}

void sbfm_tick(struct fmdrv_s* obj) {
    if (obj->g_status == 1 && obj->g_delay-- == 0)
        process_events(obj);

    // TODO: sys timer stuff
}


void sbfm_render_stereo(struct fmdrv_s* obj, float* a, float* b) {
    if (obj->g_song_timer_cnt-- == 0) {
        obj->g_song_timer_cnt = obj->g_song_timer_rate;
        sbfm_tick(obj);
    }
    adlib_get_sample_stereo(obj->m_chip, a, b);

}

void sbfm_reset(struct fmdrv_s* obj) {
    int i;

    stop_music(obj);
    opl_reset1(obj);

    for (i = 0; i < 16; ++i)
        obj->g_midi_channels[i].mute = 1;

    obj->g_num_inst = 16;
    obj->g_inst_table = default_inst_bank;

    opl_reset2(obj);
    obj->g_song_timer_fdiv = 18643;
    obj->g_transp = 0;
}

