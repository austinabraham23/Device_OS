#ifndef _PIT_H
#define _PIT_H

#include "terminal.h"

extern int8_t pid_arr[3]; // 3 terminals
extern uint8_t shell_mask[6]; 

void pit_init();
void pit_handler();
void schedule(int8_t prev_idx);

#endif
