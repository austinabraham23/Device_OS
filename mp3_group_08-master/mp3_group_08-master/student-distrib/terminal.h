#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "keyboard.h"
#include "lib.h"

#define buffer_size 128;

extern int8_t terminal_process_index;
extern uint8_t write_term_idx;
extern int8_t pid_arr[3];
extern uint8_t typing_mask[3];
extern uint8_t shell_mask[6];

/* initializes terminal variables */
int32_t terminal_open(const uint8_t* filename);
 /* does nothing rn */
int32_t terminal_close(int32_t fd);

/* reads input from terminal, returns on enter */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

/* writes string stored in buffer to terminal */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

/* initializes terminal variables and keyboard */
int32_t terminal_init();

/* helper function to clear screen */
void clear_screen();

/* function to switch between terminals*/
int32_t switch_terminal(uint32_t keycode);

/* helper function to get current cursor positions*/
int get_cursor_x();
int get_cursor_y();

/* helper function to set terminal structs in case we aren't writing to screen*/
void update_term_struct(uint8_t x, uint8_t y);

typedef struct terminal_t {
    char keyboard_buf[128]; //buffer to store inputs from keyboard
    uint32_t keyboard_idx; //index to measure how many characters have been written
    uint8_t enter_flag  : 1; //flag to signal whether enter has been pressed
    int32_t cursor_x;
    int32_t cursor_y;
    char command_buf[10][128];
    int32_t command_idx[10];
    int32_t command_pos;
    int32_t is_executing : 1;
} terminal_t;

terminal_t terminal_struct[3]; // 3 terminals

// uint32_t typing_allowed;

extern int8_t terminal_process_index;


#endif



