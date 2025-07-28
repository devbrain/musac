//
// Created by igor on 3/19/25.
//

#ifndef OPL_COMMAND_H
#define OPL_COMMAND_H


#include <stdint.h>

typedef struct opl_command {
    uint16_t Addr;
    uint8_t Data;
    double Time;
} opl_command;

#endif
