#include "paging.h"

#define PDM_SIZE         1024
/* paging_init
 * 
 * Initializes the paging system, sets up page directory and page tables.
 * Inputs: None
 * Outputs: None
 * Side Effects: Initializes the page directory and page tables.
 */
void paging_init(){

    // set up structs here
    int i;

    // set first entry in 4kB page directory
    page_directory[0].page_directory_union.kb.P = 0;
    page_directory[0].page_directory_union.kb.U_S = 0;
    page_directory[0].page_directory_union.kb.R_W = 1;
    page_directory[0].page_directory_union.kb.RSVD = 0;
    page_directory[0].page_directory_union.kb.PCD = 0;
    page_directory[0].page_directory_union.kb.PWT = 0;
    page_directory[0].page_directory_union.kb.A = 0;
    page_directory[0].page_directory_union.kb.PS = 0;
    page_directory[0].page_directory_union.kb.G = 1;
    page_directory[0].page_directory_union.kb.AVL = 0;
    
    // set all 1024 entries for 4MB page directory
    // special case for first entry with Present and Global
    for (i = 1; i < PDM_SIZE; i++){
        if(i == 1){
            page_directory[i].page_directory_union.mb.P = 1;  // different
            page_directory[i].page_directory_union.mb.G = 1;  // different
        }
        else{
            page_directory[i].page_directory_union.mb.P = 0;  // different
            page_directory[i].page_directory_union.mb.G = 0;  // different
        }
        page_directory[i].page_directory_union.mb.U_S = 0;
        page_directory[i].page_directory_union.mb.R_W = 1;
        page_directory[i].page_directory_union.mb.RSVD = 0;
        page_directory[i].page_directory_union.mb.PCD = 0;
        page_directory[i].page_directory_union.mb.PWT = 0;
        page_directory[i].page_directory_union.mb.A = 0;
        page_directory[i].page_directory_union.mb.D = 0;
        page_directory[i].page_directory_union.mb.PS = 1;
        page_directory[i].page_directory_union.mb.PAT = 0;
        page_directory[i].page_directory_union.mb.AVL = 0;
        page_directory[i].page_directory_union.mb.physical_address = i;  
    }


    // fill out all 1024 entries in the page table
    for (i = 0; i < PDM_SIZE; i++){
        first_page_table[i].P = 0;
        first_page_table[i].G = 0;
        first_page_table[i].U_S = 0;
        first_page_table[i].R_W = 1;
        first_page_table[i].PCD = 0;
        first_page_table[i].PWT = 0;
        first_page_table[i].A = 0;
        first_page_table[i].D = 0;
        first_page_table[i].PAT = 0;
        first_page_table[i].AVL = 0;
        first_page_table[i].physical_address = i;
    }

    first_page_table[184].P = 1; // video memory location
    first_page_table[184].G = 1;
    
    first_page_table[185].P = 1;    // terminal 1 paging allocation
    first_page_table[185].G = 1;

    first_page_table[186].P = 1;     // terminal 2 paging allocation
    first_page_table[186].G = 1;

    first_page_table[187].P = 1;     // terminal 3 paging allocation
    first_page_table[187].G = 1;

    // fill out 4kB page table for vidmap
    for (i = 0; i < PDM_SIZE; i++){
        vidmap_page_table[i].P = 0;
        vidmap_page_table[i].G = 1;
        vidmap_page_table[i].U_S = 0;
        vidmap_page_table[i].R_W = 1; // should be able to read and write
        vidmap_page_table[i].PCD = 0;
        vidmap_page_table[i].PWT = 0;
        vidmap_page_table[i].A = 0;
        vidmap_page_table[i].D = 0;
        vidmap_page_table[i].PAT = 0;
        vidmap_page_table[i].AVL = 0;
        vidmap_page_table[i].physical_address = i;
    }

    // set first page directory entry for present and physcial address at 4kB
    page_directory[0].page_directory_union.kb.P = 1;
    page_directory[0].page_directory_union.kb.physical_address = ((uint32_t)first_page_table) >> 12; // shift right 12 bits


    load_page_directory((unsigned int*) page_directory);
    enable_paging();
}


/* map_program_mem
 * 
 * Maps program memory for a specific process.
 * Inputs: int32_t pid - process ID of the program
 * Outputs: 0 on success, or -1 on failure.
 * Side Effects: Maps program memory for the specified process in the page directory.
 */
int32_t map_program_mem(int32_t pid){
    if(pid < 0){
        return -1; // returns failure
    }

    // sets up paging for execute/halt (32 represents the corect index for program memeory)
    page_directory[32].page_directory_union.mb.physical_address = 2 + pid;
    page_directory[32].page_directory_union.mb.P = 1;
    page_directory[32].page_directory_union.mb.U_S = 1;
    flush_tlb();

    return 0; //pass
}


/* load_program_image
 * 
 * Loads program image data into program memory at a specified offset.
 * Inputs: uint32_t offset - offset in program memory to load the data
 *         const uint8_t* buf - buffer containing the data to load
 *         uint32_t length - the number of bytes to load
 * Outputs: 0 on success, or -1 on failure.
 * Side Effects: Loads program image data into program memory at the specified offset.
 */
int32_t load_program_image(uint32_t offset, const uint8_t* buf, uint32_t length) {
    
    if (buf == NULL) return -1;  // check for NULL buffer
    
    //starting at memory at address (offset + 0x08048000 + i)
    int32_t i;
    int8_t* mem_ptr;
    int32_t starting_address = offset + 0x08048000;
    //loop through i from 0 until length (byte by byte memcopy)
    for (i = 0; i < length; i++){
        mem_ptr = (int8_t*)(starting_address + i);
        *(mem_ptr) = buf[i];
    }

    return 0; // return success
}

int32_t map_vidmap_mem(){
    page_directory[33].page_directory_union.kb.P = 1;
    page_directory[33].page_directory_union.kb.U_S = 1;
    page_directory[33].page_directory_union.kb.physical_address = ((uint32_t)vidmap_page_table) >> 12;

    vidmap_page_table[0].P = 1;
    vidmap_page_table[0].U_S = 1;
    vidmap_page_table[0].physical_address = 0xB8; //184, physical position of vmem

    flush_tlb();
    return 0;
}


// useless function going to get rid of it later
int32_t update_video_memory_paging(int8_t target_terminal){
    if(target_terminal < 0 || target_terminal > 3){
        return -1;
    }
    if(terminal_process_index == target_terminal){
        // set video memory paging to the regular physical memory of vidoe mem
        vidmap_page_table[VIDMEM_INDEX].physical_address = VIDMEM_INDEX; //184, physical position of vmem
    }else{
        vidmap_page_table[VIDMEM_INDEX].physical_address = TERM1_INDEX + target_terminal; //184, physical position of vmem
    }
    return 0;

}


