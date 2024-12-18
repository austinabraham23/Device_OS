#include "lib.h"
#include "i8259.h"
#include "pit.h"
#include "system_calls.h"
#include "x86_desc.h"
#include "paging.h"

#define MAX_PID_FREQ 1193182

#define LOHIBYTE 0x30
#define MODE_3 0x6
#define CH0_PORT 0x40
#define CTRL_PORT 0x43

int8_t terminal_process_index = -1; // so terminals yet

void pit_init(){
    cli();
    int divisor = MAX_PID_FREQ/100; //set to 100 Hz
    outb(LOHIBYTE | MODE_3, CTRL_PORT);
    outb(divisor && 0xFF, CH0_PORT);
    outb(divisor >> 8, CH0_PORT); // left shift 8
    sti();
    enable_irq(0); // irq0
}

void pit_handler(){
    //branching here, 3 options -> execute shell, do nothing (all shells), switch process to another
    int8_t start_idx = terminal_process_index;

    terminal_process_index = (terminal_process_index + 1) % 3; // calculate the index of the terminal we are running the next process on

    while(1) {
        if (terminal_process_index == start_idx) { //loop and all shells
            send_eoi(0);
            return;
        } else if (pid_arr[(uint8_t) terminal_process_index] != -1 && shell_mask[(uint8_t) pid_arr[(uint8_t)terminal_process_index]] && cur_terminal != terminal_process_index) {
            terminal_process_index = (terminal_process_index + 1) % 3; // increment, but ensure 0-3
            continue;
        }
        
        break;
    }

    schedule(start_idx);

    send_eoi(0);
}


void schedule(int8_t prev_idx) {
    // store esp, ebp, tss
    /* Context Switch (creates own context switch stack and IRET) */
    uint32_t curr_ESP;
    uint32_t curr_EBP;

    if (prev_idx != -1){
        pcb_block_t* prev_pcb = pcb_array[(uint8_t)pid_arr[(uint8_t)prev_idx]];
        // store esp, ebp, tss
        /* Context Switch (creates own context switch stack and IRET) */
        /* assmebly to store current process's ESP & EBP */
        asm volatile (
            "movl %%esp, %0      ;"
            "movl %%ebp, %1      ;" 
            : "=r" (curr_ESP), "=r" (curr_EBP)
        );
        // save current tss esp & ss0 vals
        prev_pcb->program_ESP = curr_ESP;
        prev_pcb->program_EBP = curr_EBP;
        prev_pcb->TSS_program_esp0 = tss.esp0; 
        prev_pcb->TSS_program_ss0 = tss.ss0;
    }

    if (pid_arr[(uint8_t)terminal_process_index] == -1) { // start a shell
        // execute shell
        send_eoi(0);
        execute((uint8_t *)"shell");
    } else { // context switch to existing program
        pcb_block_t* curr_pcb = pcb_array[(uint8_t)pid_arr[(uint8_t)terminal_process_index]];

        // remapping program image memory
        map_program_mem(pid_arr[(uint8_t)terminal_process_index]);

        tss.esp0 = MB4 - KB4 * pid_arr[(uint8_t)terminal_process_index] - 4;  // -4 for gap in between
        tss.ss0 = KERNEL_DS;

        asm volatile (
            "movl %0, %%esp       ;"
            "movl %1, %%ebp       ;"
            :
            : "r"(curr_pcb->program_ESP), "r"(curr_pcb->program_EBP)
        );


        // change paging of video memory to the target process's terminal buffer in physical memory
        if(terminal_process_index != cur_terminal){ //if processs is not being displayed
            vidmap_page_table[VIDMEM_INDEX].physical_address = TERM1_INDEX + terminal_process_index;
        }else{ //if process is being displayed
            vidmap_page_table[VIDMEM_INDEX].physical_address = VIDMEM_INDEX;
        }

        return;
    }
}

