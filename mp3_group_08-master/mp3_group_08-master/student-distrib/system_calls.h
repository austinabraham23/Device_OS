#ifndef _FDA_H
#define _FDA_H

#include "lib.h"

#define ELF_HEADER      4
#define ELF_0           0x7F
#define ELF_1           0x45
#define ELF_2           0x4C
#define ELF_3           0x46

#define EIP_START       24
#define EIP_HEADER      4
#define FILE_BUF_SIZE   64

#define VIDMEM_ADDR     0x00B8000 //address of vidmem
#define VIDMEM_INDEX    0xB8 //index at which to set table to
#define TERM1_INDEX     0xB9 //index of term1 vidmem
#define TERM2_INDEX     0xBA //index of term2 vidmem
#define TERM3_INDEX     0xBB //index of term3 vidmem
#define USRMEM_TOP      0x8400000 //top of usermem
#define USRMEM_BOTTOM   0x8000000 //bottom of usermem

#define MB4     0x800000
#define KB4     0x2000

#define MAX_ARGS_SIZE   128


// system calls
int32_t halt(uint8_t status);
int32_t execute(const uint8_t* command);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
int32_t open(const uint8_t* filename);
int32_t close(int32_t fd);
int32_t getargs (uint8_t* buf, int32_t nbytes);
int32_t vidmap (uint8_t** screen_start);
int32_t set_handler (int32_t signum, void* handler_address);
int32_t sigreturn (void);

// points to individual device system calls
typedef struct fops
{
    int32_t (*read_ptr)(int32_t, void*, int32_t);
    int32_t (*write_ptr)(int32_t, const void*, int32_t);
    int32_t (*open_ptr)(const uint8_t*);
    int32_t (*close_ptr)(int32_t);
} fops_t;

// file descriptor array struct
typedef struct fda_entry
{
    fops_t* fops_ptr;
    uint32_t inode_num;
    uint32_t file_pos;
    uint32_t flags : 1; // 1 bit
} fda_entry_t;

// the pcb struct
typedef struct pcb_block
{
    uint32_t processid;
    uint32_t parentid;
    uint32_t TSS_prev_esp0;
    uint16_t TSS_prev_ss0;
    uint32_t prev_ESP;
    uint32_t prev_EBP;
    uint32_t prev_EIP;
    uint8_t args_array[MAX_ARGS_SIZE];
    fda_entry_t fdarray[8]; // 6 processes and stdin/out

    uint32_t TSS_program_esp0;
    uint16_t TSS_program_ss0;
    uint32_t program_ESP;
    uint32_t program_EBP;

} pcb_block_t;

pcb_block_t* pcb_array[6]; // 6 processes possible

extern uint8_t cur_terminal;
extern int8_t terminal_process_index;

#endif 

