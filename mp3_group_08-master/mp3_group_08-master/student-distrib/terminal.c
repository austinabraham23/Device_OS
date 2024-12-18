#include "terminal.h"
#include "lib.h"
#include "paging.h"

#define FIRST_VIDMEM_PAGE       185
#define FOURKB_SIZE             4096
#define MAX_BUF_SIZE            128

uint8_t cur_terminal = 0;


/* terminal _open
 * 
 * initializes terminal specific variables and returns
 * Inputs: filename
 * Outputs: int 0 on success
 * Side Effects: None
 */
int32_t terminal_open(const uint8_t* filename){
    //set all 128 entries of buffer to 0
    terminal_t* active_term = &(terminal_struct[(uint8_t)terminal_process_index]);
    memset(active_term->keyboard_buf, 0, MAX_BUF_SIZE);
    active_term->keyboard_idx = 0;
    active_term->enter_flag = 0;
    active_term->cursor_x = 0;
    active_term->cursor_y = 0;
    active_term->is_executing = 0;
    set_cursor(active_term->cursor_x, active_term->cursor_y);
    //typing_allowed = 0;

    return 0; //success
}

/* terminal _close
 * 
 * does nothing currently
 * Inputs: int32 fd
 * Outputs: int 0 on success
 * Side Effects: None
 */
int32_t terminal_close(int32_t fd){

    return -1; //terminal cannot close, return fail
}

/* terminal_read
 * 
 * copies keyboard buffer into user buffer, returns upon a user pressing enter
 * Inputs: int32 fd, void* buf (user buffer), int32 nbytes (# of bytes to be written)
 * Outputs: number of bytes read
 * Side Effects: None
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){
    int i;
    //typing_allowed = 1;

    if(buf == NULL){
        return -1;
    }

    terminal_t* active_term = &(terminal_struct[(uint8_t)terminal_process_index]);

    while(!active_term->enter_flag){ //while enter has not been pressed
        //sti();
        for(i = 0; i < nbytes; i++){ //iterate over keyboard buffer
            //active_term->prev_command_buf[i] = active_term->keyboard_buf[i]; 
            ((char *)buf)[i] = active_term->keyboard_buf[i]; //copy char into user buffer
            if(active_term->keyboard_buf[i] == 0){ //if null character, no characters left to copy
                break;         
            }
        }
        //cli();
    }
    // if(active_term->enter_flag && active_term->is_executing!=1){
    //     for( i = 0; i < 128; i++){
    //         active_term->prev_command_buf[i] = active_term->keyboard_buf[i];
    //     }
    // }

    ((char *)buf)[i] = 10; //set last bit equal to newline

    int j, k;
    //update command_buf
    if(shell_mask[(uint8_t)pid_arr[(uint8_t)terminal_process_index]]){
        for(k = 9; k > 1; k--){
            for(j = 0; j < 128; j++){
                active_term->command_buf[k][j] = active_term->command_buf[k-1][j];
            }
            active_term->command_idx[k] = active_term->command_idx[k-1];
        }

        for(j = 0; j < 128; j++){
            active_term->command_buf[1][j] = active_term->keyboard_buf[j];
            active_term->command_buf[0][j] = NULL;
        }
        active_term->command_idx[1] = active_term->keyboard_idx;
        active_term->command_idx[0] = 0;
        active_term->command_pos = 0;
    }
    //reset keyboard_buffer, index, and enter flag
    //set all 128 entries of buffer to ascii 0
    memset(active_term->keyboard_buf, 0, MAX_BUF_SIZE);
    active_term->keyboard_idx = 0;
    active_term->enter_flag = 0;

    return i+1;
}

/* terminal_write
 * 
 * writes string stored in user buffer to terminal
 * Inputs: fd, user buffer, number of bytes to be written
 * Outputs: number of bytes written
 * Side Effects: None
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
    uint32_t i;
    if(buf == NULL){
        return -1;
    }
    cli();

    if(terminal_process_index != cur_terminal){
        write_term_idx = terminal_process_index + 1;
    } 

    for(i=0; i < nbytes; i++){ //iterate through user buffer
        if(((char *)buf)[i] == 0){ //if char is NUL skip
            continue;
        }
            putc(((char *)buf)[i]); //output char to screen
    }
    
    write_term_idx = 0;
    sti();
    return i; //# of bytes written
}

/* terminal_init
 * 
 * initializes keyboard, terminal variables and sets cursor
 * Inputs: none
 * Outputs: int 0 on success
 * Side Effects: None
 */
int32_t terminal_init(){
    keyboard_init();
    int i;
    //initializes keyboard buffer to all ascii zeros
    //set all 128 entries of buffer to ascii 0
    for(i = 0; i < 3; i++){
        terminal_t* active_term = &(terminal_struct[(uint8_t)i]);
        memset(active_term->keyboard_buf, 0, MAX_BUF_SIZE);
        memset(active_term->command_buf, 0, MAX_BUF_SIZE * 10);
        memset(active_term->command_idx, 0, 40); //40 b/c 10 4 byte ints
        active_term->keyboard_idx = 0;
        active_term->enter_flag = 0;
        active_term->command_pos = 0;
        write_term_idx = i+1;
        init_colors();
        clear_screen();
    }
    write_term_idx = 0;
    init_colors();
    clear_screen();
    
    return 0; //success
}

/* clear screen
 * 
 * helper function to clear screen
 * Inputs: none
 * Outputs: none
 * Side Effects: None
 */
void clear_screen(){
    clear();
    set_cursor(0, 0);
}

int32_t switch_terminal(uint32_t keycode){
    /* WILL NEED TO BE DONE (i think)
     * switch paging of vmem to corresponding terminal
     * ^ should just be choose which vmem addr (0xB8, 0xB9, 0xBA) to map to physical 0xB8 
     * switch screen_struct to terminal 1, 2 or 3
     * set cursor
     * done?
    */
    uint8_t terminal_num = keycode - 0x3B;
    terminal_t* new_terminal_struct;
    terminal_t* old_terminal_struct;

    //get correct new terminal struct
    if(terminal_num > 2){ // ensures only 3 terminals
        return -1;
    }
    new_terminal_struct = &(terminal_struct[terminal_num]);

    //get correct old terminal struct
    if(cur_terminal > 2){
        return -1;
    } 
    old_terminal_struct = &(terminal_struct[cur_terminal]);

    //calculate pointers for copying memory
    int8_t* video_page = (int8_t*)VIDEO;
    int8_t* old_terminal_page = (int8_t*)((FIRST_VIDMEM_PAGE + cur_terminal) << 12); // right shift 12
    int8_t* new_terminal_page = (int8_t*)((FIRST_VIDMEM_PAGE + terminal_num) << 12); // right shift 12

    // switch back video memory paging to teh direct physical mapping of video memory
    vidmap_page_table[VIDMEM_INDEX].physical_address = VIDMEM_INDEX;

    //copy vmem
    memcpy(old_terminal_page, video_page, FOURKB_SIZE);
    memcpy(video_page, new_terminal_page, FOURKB_SIZE);

    // if the current scheduled process's terminal is different than the one we switched to,
    // change the video memory paging to the scheduled process's terminal buffer
    if(terminal_process_index != terminal_num){
        vidmap_page_table[VIDMEM_INDEX].physical_address = TERM1_INDEX + terminal_process_index;
    }

    //update cur_terminal
    cur_terminal = terminal_num;

    //update cursor
    set_cursor(new_terminal_struct->cursor_x, new_terminal_struct->cursor_y);
    
    return 0;
}

int get_cursor_x(){
    terminal_t* active_term = &(terminal_struct[cur_terminal]);
    if(write_term_idx == 0){
        return active_term->cursor_x;
    }
    return terminal_struct[write_term_idx-1].cursor_x;
}

int get_cursor_y(){
    terminal_t* active_term = &(terminal_struct[cur_terminal]);
    if(write_term_idx == 0){
        return active_term->cursor_y;
    }
    return terminal_struct[write_term_idx-1].cursor_y;
}

void update_term_struct(uint8_t x, uint8_t y){
    terminal_t* curr_term = &(terminal_struct[write_term_idx - 1]);
    if(write_term_idx == 0){
        curr_term = &(terminal_struct[cur_terminal]);
        terminal_struct[cur_terminal].cursor_x = x;
        terminal_struct[cur_terminal].cursor_y = y;
    }
    curr_term->cursor_x = x;
    curr_term->cursor_y = y;
}

