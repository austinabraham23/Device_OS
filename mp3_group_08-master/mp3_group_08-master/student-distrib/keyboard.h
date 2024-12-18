#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

#define KEYBOARD_DATA       0x60
#define KEYBOARD_STATUS     0x64
#define LAST_BIT            0x01

uint8_t SHFT_PRESS;
uint8_t CPSLOCK_PRESS;
uint8_t LCTRL_PRESS;
uint8_t ALT_PRESS;

extern uint8_t write_term_idx;
extern uint8_t typing_mask[3];

/* enables irq for a keyboard */
void keyboard_init();

/* handles a keyboard interrupt and prints key */
void keyboard_handler();

/* writes a single character specified by code to screen*/
uint8_t write_keyboard_char(int32_t code, uint32_t* keyboard_index, char keyboard_buffer[128]);

/* shifts screen up by one row, leaving an empty row at the bottom*/
void shift_screen();
#endif
