#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "terminal.h"
#include "paging.h"

/* Holds a mapping from a scan code to a character being typed. */
//39 == ascii code for '
//

#define MAX_BUF_SIZE            128

unsigned char lowercase_code_table[0x3A] = {0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 
                                            0, 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 10, 17, 
                                            'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`', 15, '\\', 'z', 
                                            'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, 32};

unsigned char CAPSLOCK_code_table[0x3A] =  {0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 
                                            0, 0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 10, 17, 
                                            'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', 39, '`', 15, '\\', 'Z', 
                                            'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, 0, 0, 32};

unsigned char SHIFT_code_table[0x3A] =     {0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 
                                            0, 0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 10, 17, 
                                            'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 15, '|', 'Z', 
                                            'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, 32};

unsigned char CAPSSHIFT_code_table[0x3A] = {0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 
                                            0, 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', 10, 17, 
                                            'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '"', '~', 15, '|', 'z', 
                                            'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', 0, 0, 0, 32};

/*
 * keyboard_init
 *   DESCRIPTION: Enables keyboard interrupts
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: enables irq for a keyboard device
 */   
void keyboard_init(void) {
    enable_irq(1); // 1 for keyboard
    SHFT_PRESS = 0;
    CPSLOCK_PRESS = 0;
    LCTRL_PRESS = 0;
}

/*
 * keyboard_handler
 *   DESCRIPTION: Handles keyboard data upon a called interrupt.
 *   INPUTS: none
 *   OUTPUTS: the character typed to the terminal
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */   
void keyboard_handler(void) {
    uint32_t code;
    //mask interrupts
    cli();

    //send eoi to keyboard, which is 1
    send_eoi(1);

    //check status register, return if not available
    if (!inb(KEYBOARD_STATUS) & LAST_BIT) {
        sti();
        return;
    }
        

        // read keyboard scan code
    code = inb(KEYBOARD_DATA);


    /* SPECIAL CHARACTERS
    * backspace (0x0E) - will have to delete last char 
    * tab (0x0F) - will have to print space four times
    * LCTRL (0x1D) - clear screen
    * LSHIFT (0X2A) - set shift bool = true
    * RSHIFT (0x36) - set shift bool = true
    * CAPSLOCK (0x3A) - set capslock bool = true
    * NOTHING CHARACTERS
    * ESCAPE (0x01)
    * 0x3B - 0x80
    * IMPORTANT RELEASE CODES 
    * LSHIFT (0xAB) - set shift bool = false
    * RSHIFT (0xB6) - set shift bool = false
    * CAPSLOCK (0xBB) - set capslock bool = false
    */

    //local copy of keyboard index
    uint32_t keyboard_index;
    int i;
    // local copy of keyboard buffer
    char keyboard_buffer[128];
    int curr_screen_x = get_cursor_x();
    int curr_screen_y = get_cursor_y();

    terminal_t* active_term = &(terminal_struct[cur_terminal]);

    if(active_term->enter_flag){ //if enter was just pushed, clear keyboard buffer, index and reset flag
        keyboard_index = 0;
        memset(keyboard_buffer, 0, 128);
        active_term->enter_flag = 0;
    } else{ //otherwise, set local copy to current buffer and index
        for(i=0; i < 128; i++){
            keyboard_buffer[i] = active_term->keyboard_buf[i];
        }
        keyboard_index = active_term->keyboard_idx;
    }


    uint32_t paging_status = vidmap_page_table[VIDMEM_INDEX].physical_address;
    vidmap_page_table[VIDMEM_INDEX].physical_address = VIDMEM_INDEX;

        switch(code){
            case 0x0E: //backspace
                if((keyboard_index > 0) & ((curr_screen_x > 0) | (curr_screen_y > 0))){ //do not delete if no chars in buffer or at beginning of screen
                    if(curr_screen_x == 0){ //if at beginning of line
                        curr_screen_x = 79; //go back to end of previous line
                        curr_screen_y--;
                    } else{
                        curr_screen_x--; //otherwise go back one space on same line
                    }
                    update_term_struct(curr_screen_x, curr_screen_y);
                    putc(32); //replace char with space
                    keyboard_index--; //decrement index
                    active_term->keyboard_buf[keyboard_index] = 0; //clear char in keyboard buffer
                    active_term->keyboard_idx = keyboard_index;
                    memcpy(active_term->command_buf[0], &(keyboard_buffer), MAX_BUF_SIZE);
                    active_term->command_idx[0] = keyboard_index;
                    set_cursor(curr_screen_x, curr_screen_y); //set cursor
                }
                break;
            case 0x0F: //TAB
                putc(32); //putc four spaces
                putc(32);
                putc(32);
                putc(32);
                break;
            case 0x2A: //LSHIFT PRESS
                if(SHFT_PRESS == 0){
                    SHFT_PRESS = 1;
                }
                break;
            case 0x36: //RSHIFT PRESS
                if(SHFT_PRESS == 0){
                    SHFT_PRESS = 1;
                }
                break;
            case 0x3A: //CAPSLOCK PRESS
                if(CPSLOCK_PRESS == 0){
                    CPSLOCK_PRESS = 1;
                } else{
                    CPSLOCK_PRESS = 0;
                }
                break;
            case 0xAA: //LSHIFT RELEASE
                if(SHFT_PRESS == 1){
                    SHFT_PRESS = 0;
                }
                break;
            case 0xB6: //RSHIFT RELEASE
                if(SHFT_PRESS == 1){
                    SHFT_PRESS = 0;
                }
                break;
            case 0x1D: //LCONTROL PRESS
                if(LCTRL_PRESS == 0){
                    LCTRL_PRESS = 1;
                }
                break;
            case 0x9D: //LCONTROL RELEASE
                if(LCTRL_PRESS == 1){
                    LCTRL_PRESS = 0;
                }
                break;
            case 0x37: //NUMPAD *
                break;
            case 0x38: //LALT PRESS
                if(ALT_PRESS == 0){
                    ALT_PRESS = 1;
                }
                break;
            case 0xB8: //LALT RELEASE
                if(ALT_PRESS == 1){
                    ALT_PRESS = 0;
                }
                break;
            case 0x1C: //ENTER
                if(!typing_mask[cur_terminal]){
                    break;
                }
                if(curr_screen_y == 24){ //if at bottom of screen, shift
                    shift_screen();
                } else{
                    putc(10);
                }
                active_term->enter_flag = 1;
                break;
            case 0x48: //UP ARROW
                if(!typing_mask[cur_terminal]){
                    break;
                }
                if(active_term->command_pos >= 9){ //checks to make sure we're not at last 
                    active_term->command_pos = 9;
                    break;
                } else if(active_term->command_idx[++active_term->command_pos] == 0){ //check if we're at end of queue
                    active_term->command_pos--;
                    break;
                }
                int prev_command_pos = active_term->command_pos-1;
                if(active_term->command_pos == 1){
                    memcpy(&(active_term->command_buf[0]), active_term->keyboard_buf, MAX_BUF_SIZE);
                    active_term->command_idx[0] = active_term->keyboard_idx;
                } 

                set_cursor(curr_screen_x - active_term->command_idx[prev_command_pos], curr_screen_y);
                for(i=0; i< active_term->command_idx[prev_command_pos]; i++){
                    putc(32);    
                }
                set_cursor(curr_screen_x - active_term->keyboard_idx, curr_screen_y);
 
                puts(active_term->command_buf[active_term->command_pos]);

                //copies current command into keyboard_buf & idx
                memcpy(&(active_term->keyboard_buf), active_term->command_buf[active_term->command_pos], MAX_BUF_SIZE);
                active_term->keyboard_idx = active_term->command_idx[active_term->command_pos];

                vidmap_page_table[VIDMEM_INDEX].physical_address = paging_status;
                sti();
                return;
            case 0x50: //DOWN ARROW
                if(!typing_mask[cur_terminal]){
                    break;
                }
                if(active_term->command_pos <= 0){
                    active_term->command_pos = 0;
                    break;
                }
                int last_command_pos = active_term->command_pos;
                active_term->command_pos--;


                set_cursor(curr_screen_x - active_term->command_idx[last_command_pos], curr_screen_y);
                for(i=0; i< active_term->command_idx[last_command_pos]; i++){
                    putc(32);    
                }
                set_cursor(curr_screen_x - active_term->keyboard_idx, curr_screen_y);

                puts(active_term->command_buf[active_term->command_pos]);

                memcpy(&(active_term->keyboard_buf), active_term->command_buf[active_term->command_pos], MAX_BUF_SIZE);
                active_term->keyboard_idx = active_term->command_idx[active_term->command_pos];

                vidmap_page_table[VIDMEM_INDEX].physical_address = paging_status;
                sti();
                return;
            case 0x26: //L, MUST BE LAST CASE BEFORE DEFAULT
                if(LCTRL_PRESS){
                    clear_screen();
                    break;
                }
            default:
                write_keyboard_char(code, &keyboard_index, keyboard_buffer);
        }

    //unmask interrupts
    vidmap_page_table[VIDMEM_INDEX].physical_address = paging_status;
    sti();
}

/* write keyboard_char
 * 
 * writes inputted keyboard code to screen and keyboard buffer
 * Inputs: PS2 code, keyboard index, keyboard buffer
 * Outputs: int 1 on write, 0 on no write
 * Side Effects: None
 */
uint8_t write_keyboard_char(int32_t code, uint32_t* keyboard_index, char keyboard_buffer[128]){
    if((code == 0x3B || code == 0x3C || code == 0x3D) && ALT_PRESS){ //code is either F1, F2, or F3
        switch_terminal(code);
        return 0; //returns 0 to indicated no char was written
    }
    if(code < 0x3A && *keyboard_index < 72 && typing_mask[cur_terminal]){ //only write when code is valid and less than 128 characters have been written
        //logic to decide where to source character to print from 
        terminal_t* active_term = &(terminal_struct[cur_terminal]);
        int i;
        if(SHFT_PRESS & CPSLOCK_PRESS){
            putc(CAPSSHIFT_code_table[code]);
            keyboard_buffer[*keyboard_index] = CAPSSHIFT_code_table[code];
        } else if(SHFT_PRESS){
            putc(SHIFT_code_table[code]);
                keyboard_buffer[*keyboard_index] = SHIFT_code_table[code];
        } else if(CPSLOCK_PRESS){
            putc(CAPSLOCK_code_table[code]);
                keyboard_buffer[*keyboard_index] = CAPSLOCK_code_table[code];
        } else{
            putc(lowercase_code_table[code]);
                keyboard_buffer[*keyboard_index] = lowercase_code_table[code];
        }
        (*keyboard_index)++;
        active_term->keyboard_idx = *keyboard_index;
        //write local buffer back to screen struct (all 128 characters)
        for(i = 0; i < 128; i++){
            active_term->keyboard_buf[i] = keyboard_buffer[i];
        }
        return 1; //returns 1 to indicate character was written
    }
    return 0; //returns 0 to indicate no char was written
}
