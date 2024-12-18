#include "mouse.h"
#include "i8259.h"
#include "lib.h"
#include "paging.h"
#include "terminal.h"

uint32_t mouse_x = 0;
uint32_t mouse_y = 0;
uint8_t prev_char = 0;
uint8_t paint[3] = {0, 0, 0};
uint8_t right_click = 0;

void mouse_init(void) {
    uint8_t status;

    write_wait();
    outb(0xA8, MOUSE_STATUS);

    //set compaq status/enable irq12
    write_wait();
    outb(0x20, MOUSE_STATUS);

    //next byte is status byte
    read_wait();
    status = inb(MOUSE_DATA) | 2;
    status &= 0xDF;

    //set compaq status: send 0x60 to 0x64
    write_wait();
    outb(MOUSE_DATA, MOUSE_STATUS);

    //status to 0x60
    write_wait();
    outb(status, MOUSE_DATA);

    //aux input enable command (optional)

    //send d4 to 0x64
    write_wait();
    outb(MOUSE_SELECT, MOUSE_STATUS);
    //enable streaming f4 to 0x60
    write_wait();
    outb(0xF4, MOUSE_DATA);
    //maybe check ACK
    read_wait();
    inb(MOUSE_DATA);

    enable_irq(12); // 12 for mouse
}

void mouse_handler(void) {
    uint8_t status;
    int32_t delta_x;
    int32_t delta_y;
    uint8_t paint_flag = 0;

    send_eoi(12);

    read_wait();
    status = inb(MOUSE_DATA);

    if (((status >> 7) & INPUT_BIT) || ((status >> 6) & INPUT_BIT)) {
        return;
    };

    read_wait();
    delta_x = inb(MOUSE_DATA);

    read_wait();
    delta_y = inb(MOUSE_DATA);

    if(!typing_mask[cur_terminal] && !paint[cur_terminal]){
        return;
    }

    int32_t rel_x = delta_x - ((status << 4) & 0x100);
    int32_t rel_y = delta_y - ((status << 3) & 0x100);

    rel_x /= 4;
    rel_y /= 4;

    int32_t new_mouse_x = mouse_x + rel_x;
    int32_t new_mouse_y = mouse_y - rel_y;

    if (new_mouse_x < 0) {
        new_mouse_x = 0;
    } else if (new_mouse_x > 79) {
        new_mouse_x = 79;
    }

    if (new_mouse_y < 0) {
        new_mouse_y = 0;
    } else if (new_mouse_y > 24) {
        new_mouse_y = 24;
    }

    char* video_mem = (char*)vmem_Array[0];

    //differentiate paint or no paint
    if (((status >> 1) & INPUT_BIT)) { //right click detected
        if (!right_click && shell_mask[(uint8_t) pid_arr[cur_terminal]]) {
            //SWITCH BETWEEN PAINT AND TERMINAL

            prev_char = 32; //reset prev char

            if (paint[cur_terminal]) {
                //SWITCH BACK TO SHELL

                typing_mask[cur_terminal] = 1;
                clear_screen();
                //shift_screen();
                terminal_t* active_term = &(terminal_struct[cur_terminal]);
                active_term->enter_flag = 1;
            } else {
                //MAKE SCREEN WHITE

                typing_mask[cur_terminal] = 0;
                // draw_canvas(); //make this white
                // set_cursor(0, 0);
                clear_screen();
                paint_flag = 1;
            }

            paint[cur_terminal] = 1 - paint[cur_terminal]; //invert paint flag
        }

        right_click = 1;
    } else {
        right_click = 0;
    }

    //change paging
    uint32_t paging_status = vidmap_page_table[VIDMEM_INDEX].physical_address;
    vidmap_page_table[VIDMEM_INDEX].physical_address = VIDMEM_INDEX;

    //paint and left click pressed
    if (paint[cur_terminal] && (status & INPUT_BIT)) {
        *(uint8_t *)(video_mem + ((NUM_COLS * mouse_y + mouse_x) << 1)) = 0xDB;
        *(uint8_t *)(video_mem + ((NUM_COLS * new_mouse_y + new_mouse_x) << 1)) = 0xDB;

    //normal movement of cursor
    } else {
        //erase old cursor
        if(prev_char == 0){
            prev_char = 'S';
        }
        *(uint8_t *)(video_mem + ((NUM_COLS * mouse_y + mouse_x) << 1)) = prev_char;

        //draw new cursor
        prev_char = *(uint8_t *)(video_mem + ((NUM_COLS * new_mouse_y + new_mouse_x) << 1));
        *(uint8_t *)(video_mem + ((NUM_COLS * new_mouse_y + new_mouse_x) << 1)) = 0xDB;

    }

    if (paint_flag) {
        *(uint8_t *)(video_mem + ((NUM_COLS * 0 + 0) << 1)) = 'P';
        *(uint8_t *)(video_mem + ((NUM_COLS * 0 + 1) << 1)) = 'A';
        *(uint8_t *)(video_mem + ((NUM_COLS * 0 + 2) << 1)) = 'I';
        *(uint8_t *)(video_mem + ((NUM_COLS * 0 + 3) << 1)) = 'N';
        *(uint8_t *)(video_mem + ((NUM_COLS * 0 + 4) << 1)) = 'T';
    }

    //restore paging
    vidmap_page_table[VIDMEM_INDEX].physical_address = paging_status;

    mouse_x = new_mouse_x;
    mouse_y = new_mouse_y;

}

//helpers for mouse_init()

void read_wait() { //delay to ensure you can read from port 0x60
    int32_t delay = 999999;
    while ((!inb(MOUSE_STATUS) & INPUT_BIT) && (--delay));
}

void write_wait() { //delay before writing to 0x64 or 0x60
    int32_t delay = 999999;
    while ((inb(MOUSE_STATUS) & OUTPUT_BIT) && (--delay));
}
