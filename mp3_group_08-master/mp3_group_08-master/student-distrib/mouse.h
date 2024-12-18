#ifndef _MOUSE_H
#define _MOUSE_H

#include "types.h"

#define MOUSE_DATA 0x60
#define MOUSE_STATUS 0x64
#define MOUSE_SELECT 0xD4
#define INPUT_BIT 0x1
#define OUTPUT_BIT 0x2
#define MOUSE_BITS 0x21
#define RANDOM 0x22

extern uint32_t vmem_Array[4];
extern uint8_t typing_mask[3];
extern uint8_t shell_mask[6]; 

void mouse_init();

void mouse_handler();

void read_wait();

void write_wait();

#endif
