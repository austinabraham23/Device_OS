#ifndef _PAGING_INIT_H
#define _PAGING_INIT_H

/* Types */
#include "types.h"

#define VIDMEM_INDEX    0xB8 //index at which to set table to
#define TERM1_INDEX     0xB9 //index of term1 vidmem

extern void load_page_directory(unsigned int* page_directory_addr);
extern void enable_paging();
extern void flush_tlb();

void paging_init();
int32_t map_program_mem(int32_t pid);
int32_t load_program_image(uint32_t offset, const uint8_t* buf, uint32_t length);
int32_t map_vidmap_mem();
int32_t update_video_memory_paging(int8_t target_terminal);

/* Struct for Page Directory Elements */
typedef struct page_directory_entry_kb {
    uint32_t P      : 1;
    uint32_t R_W    : 1;
    uint32_t U_S    : 1;
    uint32_t PWT    : 1;
    uint32_t PCD    : 1;
    uint32_t A      : 1;
    uint32_t RSVD   : 1;
    uint32_t PS     : 1;
    uint32_t G      : 1;
    uint32_t AVL    : 3;
    uint32_t physical_address : 20;
} page_directory_entry_kb_t;

/* Struct for Page Directory Elements */
typedef struct page_directory_entry_mb {
    uint32_t P      : 1;
    uint32_t R_W    : 1;
    uint32_t U_S    : 1;
    uint32_t PWT    : 1;
    uint32_t PCD    : 1;
    uint32_t A      : 1;
    uint32_t D      : 1;
    uint32_t PS     : 1;
    uint32_t G      : 1;
    uint32_t AVL    : 3;
    uint32_t PAT    : 1;
    uint32_t RSVD   : 9;
    uint32_t physical_address : 10;
} page_directory_entry_mb_t;

/* Struct for Page Table Entry */
typedef struct page_table_entry {
    uint32_t P      : 1;
    uint32_t R_W    : 1;
    uint32_t U_S    : 1;
    uint32_t PWT    : 1;
    uint32_t PCD    : 1;
    uint32_t A      : 1;
    uint32_t D      : 1;
    uint32_t PAT    : 1; 
    uint32_t G      : 1;
    uint32_t AVL    : 3;
    uint32_t physical_address: 20;

} page_table_entry_t;

typedef struct __attribute__((packed)) pd {
    union{
        struct page_directory_entry_kb kb;
        struct page_directory_entry_mb mb;
    } page_directory_union;
} page_directory_entry_t;

page_directory_entry_t page_directory[1024] __attribute__((aligned(4096)));  // will have to change these later and implement a proper page allocator

page_table_entry_t first_page_table[1024] __attribute__((aligned(4096)));
page_table_entry_t vidmap_page_table[1024] __attribute__((aligned(4096)));

extern int8_t terminal_process_index;

#endif /* _PAGING_INIT_H */
