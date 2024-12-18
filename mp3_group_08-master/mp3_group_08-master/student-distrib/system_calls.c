#include "system_calls.h"
#include "lib.h"
#include "file_system.h"
#include "rtc.h"
#include "paging.h"
#include "terminal.h"
#include "x86_desc.h"

/* Global variables for FDA */
const static uint8_t ELF_MAGIC[4] = {ELF_0, ELF_1, ELF_2, ELF_3};

static int32_t halt_value; // return value inside halt that will be put into execute

//int32_t pid = -1; //maybe retarded, initialized to -1 so during 
                  //execute gets incremented to 0
                  
// static int32_t shell_count = 0;
uint8_t shell_mask[6] = {0, 0, 0, 0, 0, 0};
uint8_t typing_mask[3] = {1, 1, 1};
static uint8_t pid_mask[6] = {0, 0, 0, 0, 0, 0};
int8_t pid_arr[3] = {-1, -1, -1}; //circular linked list

// fops pointer associated with 4 main system calls for RTC
fops_t rtc_fops = {
    rtc_read,
    rtc_write,
    rtc_open,
    rtc_close
};

// fops pointer associated with 4 main system calls for file system
fops_t file_fops = {
    file_read,
    file_write,
    file_open,
    file_close
};

// fops pointer associated with 4 main system calls for directory
fops_t dir_fops = {
    directory_read,
    directory_write,
    directory_open,
    directory_close
};

// fops pointer associated keyboard read
fops_t read_fops = {
    terminal_read,
    NULL,
    NULL,
    NULL
};

// fops pointer associated keyboard write
fops_t write_fops = {
    NULL,
    terminal_write,
    NULL,
    NULL
};

/* local functions */
int32_t copy_program_image(uint32_t inode);
int32_t create_pcb(int32_t next_pid);
// int32_t user1_signal_handler();
// int32_t alarm_signal_handler();
// int32_t interrupt_signal_handler();
// int32_t segfault_signal_handler();
// int32_t div_zero_signal_handler();

/* halt
 * 
 * Terminates a process, restores parent's data, and returns a specified value to its parent process.
 * Inputs: uint8_t status - the status value to return to the parent process
 * Outputs: Does not return (jumps to a different part of the code)
 * Side Effects: Restores parent data, closes file descriptors, updates TSS, and jumps to execute return.
 */
int32_t halt (uint8_t status){
    int i; // for loops

    //varun: do i need to check -1 cases???
    int32_t curr_pid = pid_arr[(uint8_t)terminal_process_index];

    pcb_block_t* curr_pcb = pcb_array[curr_pid]; // sets up pcb for specific function
    // could be here
    int32_t pid_temp = curr_pcb->parentid; // holds temporary pid value for work inside function

    //make sure you can't close base shell, if try, run base shell again
    if (curr_pid < 3){
        uint32_t prev_eip = curr_pcb->prev_EIP;
        asm volatile (
            "pushl %1            ;" // Push USER_DS
            "pushl $0x83FFFFC    ;" // Push esp, always 132MB - byte
            "pushfl              ;" // Push flags
            "pushl %0            ;" // Push USER_CS
            "pushl %2            ;" // Push prev_eip, will run shell again
            "iret                ;"
            :
            : "r"(USER_CS), "r"(USER_DS), "r"(prev_eip)
        );
    }

    /* Restore parent paging */
    if (-1 == map_program_mem(pid_temp)) {
        halt_value = -1; // return failure
        asm volatile("jmp finish_execute    ;"); // jumps to execute
    }

    //decrement shell count
    if (shell_mask[curr_pid]) {
        shell_mask[curr_pid] = 0; // setting to inactive
    }

    // resets all flags (except stdin & stdout)
    for (i = 2; i < 8; i++) {
        close(i);
    }
 
    /* Write parent process' info back to TSS (esp0) */
    tss.esp0 = curr_pcb->TSS_prev_esp0;
    tss.ss0 = curr_pcb->TSS_prev_ss0;

    pid_mask[curr_pid] = 0;
    typing_mask[(uint8_t)terminal_process_index] = 1;
    pid_arr[(uint8_t)terminal_process_index] = pid_temp;
    terminal_t* active_term = &(terminal_struct[cur_terminal]);
    active_term->is_executing = 0;

    /* Jump to execute return (returns value to parent execute function) */
    halt_value = (int32_t) status; // returns status
    asm volatile (
        "movl %0, %%esp       ;"
        "movl %1, %%ebp       ;"
        "jmp finish_execute   ;"
        :
        : "r"(curr_pcb->prev_ESP), "r"(curr_pcb->prev_EBP)
    );

    return 0;
};


/* execute
 * 
 * Executes a given command by loading and running an executable program.
 * Inputs: const uint8_t* command - the command to execute
 * Outputs: Does not return (jumps to a different part of the code)
 * Side Effects: Loads and runs the specified executable program.
 */
int32_t execute (const uint8_t* command){
    cli();
    dentry_t entry;
    int32_t parse = 0;
    int32_t fnamend = 0;
    uint8_t c, i;
    uint8_t filename[FILENAME_LEN + 1];
    uint8_t shell_name[] = "shell";
    uint8_t pingpong_name[] = "pingpong";
    uint8_t fish_name[] = "fish";
    uint8_t arguments[MAX_ARGS_SIZE]; //arguments field w/ max size that can be written in terminal
    uint8_t argflag = 0;
    uint32_t eip_buf;
    int32_t pid_temp = -1;
    uint8_t shell_flag = 0; //boolean
    uint8_t typing_flag = 1;

    for (i = 0; i < 6; i++) {
        if (pid_mask[i] == 0) { //available
            pid_temp = i;
            break;
        }
    }

    if (pid_temp == -1 || command == NULL) return -1; // no pids left and null check

    arguments[0] = '\0';

    //parse first arg
    while ('\0' != (c = command[parse])) {
        if (c == ' ' && argflag == 0){
            argflag = 1;
            filename[parse] = '\0';
            parse++;
            fnamend = parse;
            continue;
        }
        if(argflag){
            arguments[parse - fnamend] = c;
        } else{
            filename[parse] = c;
        }
        parse++;
        if(parse == FILENAME_LEN && !argflag){
            break; //more than 124 chars fix, will eventually need to be changed
        }
    }

    if(!argflag){
        filename[parse] = '\0';
    } 
    arguments[parse - fnamend] = '\0';

    // check if shell
    if (0 == strncmp( (int8_t*) filename, (int8_t*) shell_name, 6)) { // 6 is the size of the filename
        shell_flag = 1;
    } else if (0 == strncmp( (int8_t*) filename, (int8_t*) pingpong_name, 9) || 0 == strncmp( (int8_t*) filename, (int8_t*) fish_name, 5)) {
        typing_flag = 0;
    }  
 
    if (-1 == read_dentry_by_name(filename, &entry)) return -1;
    
    if (ELF_HEADER != read_data(entry.inode_num, 0, filename, ELF_HEADER)) return -1;

    // get eip value
    if (EIP_HEADER != read_data(entry.inode_num, EIP_START, (uint8_t *)&eip_buf, EIP_HEADER)) return -1; // read bytes 24-27 in the executable file to get eip value

    /* executable check */
    for (i = 0; i < ELF_HEADER; i++) {
        if (ELF_MAGIC[i] != filename[i]){
            return -1; // return failure
        }
    }

    /* Create PCB */

    if( -1 == create_pcb(pid_temp)) return -1; // creates the pcb
    pcb_block_t* curr_pcb = pcb_array[pid_temp]; // sets up execute pcb

    /* set up program paging */

    if (-1 == map_program_mem(pid_temp)) return -1; // return failure 

    /*User-Level Progam Loader */ 

    //copy file data to memory
    if (-1 == copy_program_image(entry.inode_num)) return -1; // return failure


    //varun: maybe does not copy nul byte? could use strcpy, also might wanna move this after the 
    i = 0;
    while(arguments[i] != '\0'){
        curr_pcb->args_array[i] = arguments[i];
        i++;
    }

    /* Context Switch (creates own context switch stack and IRET) */
    uint32_t curr_ESP;
    uint32_t curr_EBP;

    /* assmebly to store current process's ESP & EBP */
    asm volatile (
        "movl %%esp, %0      ;"
        "movl %%ebp, %1      ;" 
        : "=r" (curr_ESP), "=r" (curr_EBP)
    );

    // sets prev_EBP & prev_ESP
    curr_pcb->prev_EBP = curr_EBP;
    curr_pcb->prev_ESP = curr_ESP;

    // save current tss esp & ss0 vals
    curr_pcb->TSS_prev_esp0 = tss.esp0; 
    curr_pcb->TSS_prev_ss0 = tss.ss0;

    tss.esp0 = MB4 - KB4 * pid_temp - 4;  // -4 for gap in between
    tss.ss0 = KERNEL_DS;

    //save this in case we try to exit shell
    curr_pcb->prev_EIP = eip_buf;
    
    //update shell vals
    if (shell_flag) {
        shell_mask[pid_temp] = 1; // setting to active
    }

    typing_mask[cur_terminal] = typing_flag;

    pid_mask[pid_temp] = 1; // set global pid active

    if (pid_temp < 3) {
        pid_arr[pid_temp] = pid_temp;
    } else {
        pid_arr[cur_terminal] = pid_temp;
    }
    terminal_t* active_term = &(terminal_struct[cur_terminal]);
    active_term->is_executing = 1;

    // iret push function here
    asm volatile (
        "pushl %1            ;" // Push USER_DS
        "pushl $0x83FFFFC    ;" // Push esp, always 132MB - byte
        "sti                 ;"
        "pushfl              ;" // Push flags
        "pushl %0            ;" // Push USER_CS
        "pushl %2            ;" // Push eip_buf
        "iret                ;"
        "finish_execute:     ;" // label for jump in halt
        :
        : "r"(USER_CS), "r"(USER_DS), "r"(eip_buf)
    );

    return halt_value; // return halt status
};


/* read
 * 
 * Reads data from the specified file descriptor.
 * Inputs: int32_t fd - file descriptor index
 *         void* buf - buffer to store the read data
 *         int32_t nbytes - number of bytes to read
 * Outputs: The number of bytes read on success, or -1 on failure.
 * Side Effects: Reads data from the file descriptor and updates the buffer.
 */
int32_t read (int32_t fd, void* buf, int32_t nbytes){
    int8_t pid = pid_arr[(uint8_t)terminal_process_index];
    pcb_block_t* curr_pcb = pcb_array[(uint8_t)pid]; // set up read pcb
    if (curr_pcb == NULL || buf == NULL || nbytes < 0 || fd < 0 || fd > 7) return -1; // return faliure
    fda_entry_t curr_fda = curr_pcb->fdarray[fd]; // set up current read fda

    // checks if the fda is active if not returns fail
    if (curr_fda.flags == 0){
        return -1; // return failure
    }

    fops_t curr_fops = *(curr_fda.fops_ptr);

    if(*curr_fops.read_ptr == NULL) return -1;

    return (*(curr_fops.read_ptr))(fd, buf, nbytes); // return function to be called
};


/* write
 * 
 * Writes data to the specified file descriptor.
 * Inputs: int32_t fd - file descriptor index
 *         const void* buf - buffer containing the data to write
 *         int32_t nbytes - number of bytes to write
 * Outputs: The number of bytes written on success, or -1 on failure.
 * Side Effects: Writes data to the file descriptor based on the provided buffer and updates the file.
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes){
    int8_t pid = pid_arr[(uint8_t)terminal_process_index];
    pcb_block_t* curr_pcb = pcb_array[(uint8_t)pid]; // set up write pcb
    if (curr_pcb == NULL || buf == NULL || nbytes < 0 || fd < 0 || fd > 7) return -1; // return failure
    fda_entry_t curr_fda = curr_pcb->fdarray[fd]; // set up write FDA

    // checks if the fda is active if not returns fail
    if (curr_fda.flags == 0){
        return -1; // return failure
    }

    fops_t curr_fops = *(curr_fda.fops_ptr);

    if(*curr_fops.write_ptr == NULL) return -1;

    return (*(curr_fops.write_ptr))(fd, buf, nbytes); // return function to be called
};


/* open
 * 
 * Opens a file or device by setting up a file descriptor for it.
 * Inputs: const uint8_t* filename - the name of the file or device to open
 * Outputs: The file descriptor index on success, or -1 on failure.
 * Side Effects: Sets up a file descriptor for the opened file or device.
 */
int32_t open (const uint8_t* filename){
    int8_t pid = pid_arr[(uint8_t)terminal_process_index];
    int32_t fd;
    dentry_t entry;
    int32_t set = 0; // initialize set

    pcb_block_t* curr_pcb = pcb_array[(uint8_t)pid]; // set up open pcb
    if (curr_pcb == NULL) return -1; // return failure

    // set FDA flags
    for (fd = 2; fd < 8; fd++) {
        if (!curr_pcb->fdarray[fd].flags) {
            set = 1;
            break;
        }
    }

    // check filename
    if (!set || (-1 == read_dentry_by_name(filename, &entry))) return -1;

    // set up pcb fdarray values
    curr_pcb->fdarray[fd].file_pos = 0;
    curr_pcb->fdarray[fd].flags = 1;
    curr_pcb->fdarray[fd].inode_num = entry.inode_num;

    // set fops for each type of open call
    switch(entry.filetype) {
        case 0:             // RTC
            curr_pcb->fdarray[fd].fops_ptr = &rtc_fops;
            break;
        case 1:             // Directory
            curr_pcb->fdarray[fd].fops_ptr = &dir_fops;
            break;
        case 2:             // File
            curr_pcb->fdarray[fd].fops_ptr = &file_fops;
    }

    // check for failure
    if (-1 == (*(curr_pcb->fdarray[fd].fops_ptr->open_ptr))(filename)){
        curr_pcb->fdarray[fd].flags = 0;
        return -1;
    }

    return fd; // returns fd for open call
};


/* close
 * 
 * Closes a file descriptor, marking it as inactive and releasing associated resources.
 * Inputs: int32_t fd - file descriptor index to close
 * Outputs: 0 on success, or -1 on failure.
 * Side Effects: Marks the file descriptor as inactive and releases associated resources.
 */
int32_t close (int32_t fd){
    // tries to close an invalid decriptor
    if (fd < 2 || fd > 7){
        return -1; // return failure
    }
    int8_t pid = pid_arr[(uint8_t)terminal_process_index];
    // sets flags to 0 to show it is inactive
    pcb_block_t* curr_pcb = pcb_array[(uint8_t)pid]; // set up close pcb
    if (curr_pcb == NULL) return -1; // returns failure
    fda_entry_t curr_fda = curr_pcb->fdarray[fd];
    if(curr_fda.flags == 0){
        return -1;
    }
    curr_pcb->fdarray[fd].flags = 0;
    fops_t curr_fops = *(curr_fda.fops_ptr);
    
    // calls the specificed close function which returns -1 or 0 based on success
    return (*(curr_fops.close_ptr))(fd);
};


/* getargs
 * 
 * Retrieves the arguments stored in the process control block (PCB).
 * Inputs: uint8_t* buf - Buffer to store the arguments.
 *         int32_t nbytes - Maximum number of bytes to copy.
 * Outputs: 0 on success, or -1 on failure.
 * Side Effects: Copies arguments from the PCB to the provided buffer and clears the arguments in the PCB.
 */
int32_t getargs (uint8_t* buf, int32_t nbytes){
    int i;
    int8_t pid = pid_arr[(uint8_t)terminal_process_index];
    pcb_block_t* curr_pcb = pcb_array[(uint8_t)pid];
    
    // check for valid PCB and nbytes
    if(curr_pcb == NULL || curr_pcb->args_array[0] == '\0' || nbytes < 1) return -1; //fail if cannot get pcb

    // copy arg into buffer
    for(i=0; i < nbytes; i++){
        if(curr_pcb->args_array[i] != '\0'){
            buf[i] = curr_pcb->args_array[i];
        } else{
            buf[i] = '\0';
            break;
        }
    }

    // varun: could this technically go out of bounds? because if the break cond is never met, then i = nbytes and OOB right???
    // check for potential out of bounds condition
    if(buf[i] != '\0'){
        return -1; // return failure
    }

    // clears the arg in PCB
    memset(curr_pcb->args_array, '\0', MAX_ARGS_SIZE);

    return 0; // return success
}


/* vidmap
 * 
 * Maps the video memory into user space.
 * Inputs: uint8_t** screen_start - Pointer to store the starting address of the mapped video memory.
 * Outputs: 0 on success, or -1 on failure.
 * Side Effects: Maps video memory into user space, updating page tables and TLB.
 */
int32_t vidmap (uint8_t** screen_start){
    // check for valid screen_start
    if(screen_start == NULL || (uint32_t)screen_start < USRMEM_BOTTOM || (uint32_t)screen_start > USRMEM_TOP){
        return -1; // return failure
    }

    // set up page directory entry for video mem
    page_directory[0].page_directory_union.kb.P = 1;
    page_directory[0].page_directory_union.kb.U_S = 1;
    page_directory[0].page_directory_union.kb.R_W = 1;
    page_directory[0].page_directory_union.kb.physical_address = ((uint32_t)vidmap_page_table) >> 12;

    // set up page table entry for vid mem
    vidmap_page_table[VIDMEM_INDEX].P = 1;
    vidmap_page_table[VIDMEM_INDEX].U_S = 1;
    vidmap_page_table[VIDMEM_INDEX].R_W = 1;
    vidmap_page_table[VIDMEM_INDEX].physical_address = VIDMEM_INDEX; //184, physical position of vmem

    // set up page table entry for term1 mem
    vidmap_page_table[TERM1_INDEX].P = 1;
    vidmap_page_table[TERM1_INDEX].U_S = 1;
    vidmap_page_table[TERM1_INDEX].R_W = 1;
    vidmap_page_table[TERM1_INDEX].physical_address = TERM1_INDEX; //185, physical position of terminal 1

    // set up page table entry for term2 mem
    vidmap_page_table[TERM2_INDEX].P = 1;
    vidmap_page_table[TERM2_INDEX].U_S = 1;
    vidmap_page_table[TERM2_INDEX].R_W = 1;
    vidmap_page_table[TERM2_INDEX].physical_address = TERM2_INDEX; //186, physical position of terminal 2

    // set up page table entry for term3 mem
    vidmap_page_table[TERM3_INDEX].P = 1;
    vidmap_page_table[TERM3_INDEX].U_S = 1;
    vidmap_page_table[TERM3_INDEX].R_W = 1;
    vidmap_page_table[TERM3_INDEX].physical_address = TERM3_INDEX; //187, physical position of terminal 3

    flush_tlb(); // flush tlb to update table changes

    *screen_start = (uint8_t*)VIDMEM_ADDR; // set the screen start to the starting address of vid mem
    return 0; // return success
}


/* set_handler
 * 
 * Changes the default action taken when a signal is received.
 * Inputs: int32_t signum - Signal number specifying which signal's handler to change.
 *         void* handler_address - Pointer to a user-level function to be run when the signal is received.
 * Outputs: 0 on success, or -1 on failure.
 * Side Effects: Changes the signal handler for the specified signal.
 */
int32_t set_handler (int32_t signum, void* handler_address){
    
    // if(handler_address == 0) return -1;

    // switch(signum){
    //     case 0:
    //         handler_address = *div_zero_signal_handler();
    //     case 1:
    //         handler_address = *segfault_signal_handler();
    //     case 2:
    //         handler_address = *interrupt_signal_handler();
    //     case 3:
    //         handler_address = *alarm_signal_handler();
    //     case 4:
    //         handler_address = *user1_signal_handler();
    //     default:
    //         return -1;

    // }
    // return 0;
    return -1;

}


/* sigreturn
 * 
 * Copies the hardware context from the user-level stack back onto the processor.
 * Returns the hardware context's EAX value.
 * Inputs: None
 * Outputs: EAX value from the hardware context.
 * Side Effects: Overwrites the kernel's copy of the process's hardware context saved on the kernel stack.
 */
int32_t sigreturn (void){
    return -1; //immediately return failure
}


/* create_pcb
 * 
 * Creates a Process Control Block (PCB) for a new process.
 * Inputs: int32_t pid - process ID of the new process
 * Outputs: 0 on success, or -1 on failure.
 * Side Effects: Initializes a PCB for a new process and stores it in the PCB array.
 */
int32_t create_pcb(int32_t next_pid){
    int32_t w;
    if(next_pid < 0){
        return -1; // returns failure
    }


    // sets up address by shifting 8 right 20, and the new pid right 13
    uint32_t addr = (8 << 20) - ((next_pid + 1) << 13);
    pcb_block_t* local_pcb = (pcb_block_t*) addr;
    
    // sets up pcb struct
    local_pcb->parentid = (next_pid < 3) ? next_pid : pid_arr[cur_terminal];
    local_pcb->processid = next_pid;
    local_pcb->TSS_prev_esp0 = 0;
    local_pcb->TSS_prev_ss0 = 0;
    local_pcb->prev_EBP = 0;
    local_pcb->prev_ESP = 0;
    local_pcb->fdarray[0].fops_ptr = &(read_fops);
    local_pcb->fdarray[1].fops_ptr = &(write_fops);
    memset(local_pcb->args_array, '\0', MAX_ARGS_SIZE);

    // sets pcb fd array values
    for(w=0; w < 8; w++){
        if(w < 2){
            local_pcb->fdarray[w].flags = 1;
            local_pcb->fdarray[w].file_pos = 0;
            local_pcb->fdarray[w].inode_num = 0;
        } else{
            local_pcb->fdarray[w].flags = 0;
        }
    }
    
    // sets array

    pcb_array[next_pid] = local_pcb;
    return 0; // return success
}


/* copy_program_image
 * 
 * Copies the program image data from the file system to the program memory.
 * Inputs: uint32_t inode - inode number of the program image
 * Outputs: 0 on success, or -1 on failure.
 * Side Effects: Copies the program image data to program memory.
 */
int32_t copy_program_image(uint32_t inode) {
    uint8_t buf[FILE_BUF_SIZE];
    uint32_t offset = 0; // initializes offset
    int32_t cnt;

    // goes byte by byte, ensuring not failure to copy
    while (0 != (cnt = read_data(inode, offset, buf, FILE_BUF_SIZE))) {
        if (-1 == cnt || -1 == load_program_image(offset, buf, cnt)) return -1; // return failure
        offset += cnt;
    }

    return 0; // return success
}

