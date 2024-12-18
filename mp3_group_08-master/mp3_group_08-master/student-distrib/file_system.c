#include "file_system.h"
#include "lib.h"

int32_t dir_entry_num;
int32_t inodes_num;
int32_t data_blocks_num;
boot_block_t* boot_block_ptr;       /* start of boot block ptr */
inode_t* inode_ptr;                 /* start of inode arr ptr */
data_block_t* data_block_ptr;       /* start of data block arr ptr */


/* file_sys_init
 * 
 * sets variables for the file system used later
 * Inputs: boot_block_start
 * Outputs: None
 * Side Effects: None
 */
void file_sys_init(boot_block_t* boot_block_start){
    if (!boot_block_start) return;

    //get pointer top mod->start , aka boot block
    boot_block_ptr = boot_block_start;

    // fill in bloot_block_struct for dir count, inode_count and data count
    dir_entry_num = boot_block_ptr->dir_count;
    inodes_num = boot_block_ptr->inode_count;
    data_blocks_num = boot_block_ptr->data_count;

    //get pointers to inode array and data block array
    inode_ptr = (inode_t*)(boot_block_start + 1);
    data_block_ptr = (data_block_t*) (inode_ptr + (1 * inodes_num));
}

/* read_dentry_by_name
 * 
 * find the directory entry associated with the specified filename and assigns it to the passed in dentry
 * Inputs: fname -- file name we are seraching for
 *          dentry -- dentry strcut pointer we are assigning to the dentry we match with teh target filename
 * Outputs: returns 0 if a match is found, -1 otherwise
 * Side Effects: fills in the passed in dentry
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
    int32_t i, s, match;
    dentry_t* entry;
    uint8_t input_char, file_char;

    //check for null ptr
    if (fname == NULL || strlen((int8_t*) fname) > FILENAME_LEN) return -1;

    // loop through all of the dentries to find a name match
    for (i = 0; i < dir_entry_num; i++) {
        entry = &boot_block_ptr->direntries[i]; // grab current entry
        s = 0;
        match = 1; // default to match

        //compare fname to entry filename
        while (s < FILENAME_LEN) {
            input_char = fname[s];
            file_char = entry->filename[s];

            if (input_char != file_char) { //no match, need to continue outer
                match = 0;
                break;
            }

            if (input_char == '\0') break; //end of string

            s++; // check next char
        }

        if (!match) continue;

        //found right entry
        *dentry = *entry;
        return 0;
    }

    return -1; // no matches
}

/* read_dentry_by_index
 * 
 * find the directory entry associated with the specified index and assigns it to the passed in dentry
 * Inputs: index -- specific index we are grabbing the dentry from
 *          dentry -- dentry strcut pointer we are assigning to the dentry we match with teh target index
 * Outputs: returns 0 if the dentry at the index is found, -1 otherwise
 * Side Effects: fills in the passed in dentry
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {

    //check index range
    if (index >= dir_entry_num) return -1;

    // sent dentry to the dentry at the specified index
    *dentry = boot_block_ptr->direntries[index];
    return 0;
}


/* read_data
 * 
 * reads a certain amopunt of bytes of data from the file at teh specified inode. 
 * Inputs: inode -- inode of the file we are reading from
 *          offset -- the starting point in the file that we start reading from
 *          buf -- the buffer we are filling with the data we are reading
 *          length -- the amount of bytes we want to read from teh file
 * Outputs: returns read, the number of bytes read from the file, or -1 otherwise if there's an error
 * Side Effects: fills in the passed in buf with data
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    inode_t* curr_inode;
    int32_t file_size, data_block_idx;
    int32_t read = 0;
    uint32_t data_block_num_idx, data_block_offset;
    data_block_t* data;

    //check inode validity
    if (inode >= inodes_num) return -1;
    if (buf == NULL) return -1;  // check for NULL buffer

    //get current inode and data indexes
    curr_inode = inode_ptr + inode;
    file_size = curr_inode->length;
    data_block_num_idx = offset / BLOCK_BYTE_SIZE;
    data_block_offset = offset % BLOCK_BYTE_SIZE;

    //offset is out of bounds, return 0
    if (data_block_num_idx >= 1023) return 0;

    data_block_idx = curr_inode->data_block_num[data_block_num_idx];

    //check data block index validity
    if (data_block_idx < 0 || data_block_idx >= data_blocks_num) return -1;

    data = data_block_ptr + data_block_idx;

    //loop through bytes
    while (offset < file_size && read < length) {
        if (data_block_offset == BLOCK_BYTE_SIZE) { //move to next data block
            data_block_idx = curr_inode->data_block_num[++data_block_num_idx];

            //check data block index validity
            if (data_block_idx < 0 || data_block_idx >= data_blocks_num) return -1;

            data = data_block_ptr + data_block_idx;
            data_block_offset = 0;
        }

        //read a byte and continue
        buf[read] = data->byte[data_block_offset];

        data_block_offset++;
        read++;
        offset++;
    }

    return read;
}


/* get_file_size
 * 
 * Retrieves the size of a file identified by its inode.
 * Inputs: inode -- the inode number of the file
 * Outputs: the size of the file in bytes, or -1 if the inode is invalid
 * Side Effects: None
 */
int32_t get_file_size(uint32_t inode) {
    if (inode >= inodes_num) return -1; //check inode valid

    inode_t* curr = inode_ptr + inode; //find inode block

    return curr->length;
}


// FILE SYSTEM DRIVER FOR FILES

/* file_open
 * 
 * initializes varibles for file read, and finds the dentry of the filename we input
 * Inputs: filename -- file name we are opening 
 * Outputs: returns 0 if file is opened, -1 it doesn't exist
 * Side Effects: None
 */
int32_t file_open(const uint8_t* filename){
    if(filename == NULL){   // check for null filename
        return -1;
    }

    //from filename, find inode
    dentry_t entry;
    if (read_dentry_by_name(filename, &entry) != 0){  // find dentry for filename
        return -1;
    }

    if (entry.filetype != 2) return -1;

    return 0;
}

/* file_close
 * 
 * closes the opened file
 * Inputs: fd -- file descriptor index we are closing
 * Outputs: returns 0, or -1 if fd is invalid
 * Side Effects: None
 */
int32_t file_close(int32_t fd){
    if(fd < 0 || fd >= 8){ // check fd index
        return -1; 
    }
    return 0;
}

/* file_write
 * 
 * does nothing, this is a read only file system
 * Inputs: fd -- file descriptor index we are writing to
 *          buf -- the buffer filled with what you want to write
 *          nbytes - amount of bytes you want to read
 * Outputs: returns -1 bc this is a read only file system
 * Side Effects: None
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes){
    if (buf == NULL) return -1;  // check for NULL buffer
    if (fd < 0 || fd >= 8) return -1;  // check fd index

    return -1;  // read only file system. Return -1
}

/* file_read
 * 
 * reads data from the target file
 * Inputs: fd -- file descriptor index we are reading from
 *          buf -- buffer we fill in with the data we are reading
 *          nbytes -- the number of bytes we want to read
 * Outputs: returns 0, or -1 if fd is invalid
 * Side Effects: None
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes){
    if (buf == NULL) return -1;  // check for NULL buffer
    if (fd < 0 || fd >= 8) return -1;  // check fd index

    uint32_t num_bytes_read;

    int8_t pid = pid_arr[(uint8_t)terminal_process_index];

    fda_entry_t* curr_file = &pcb_array[(uint8_t)pid]->fdarray[fd];
    
    num_bytes_read = read_data(curr_file->inode_num, curr_file->file_pos, buf, nbytes);  // read the file data, and check the number of bytes read

    curr_file->file_pos += num_bytes_read; // increment the file_read_index to read from where you left off next time

    return num_bytes_read;
}


// FILE SYSTEM DRIVER FOR DIRECTORIES


/* directory_open
 * 
 * initializes varibles for directory read, and finds the dentry of the directory file name we input
 * Inputs: filename -- file name we are opening, which should be a directory 
 * Outputs: returns 0 if directory is opened, -1 if its not a directory or doesn't exist
 * Side Effects: None
 */
int32_t directory_open(const uint8_t* filename){
    if(filename == NULL){   // check for null filename
        return -1;
    }

    dentry_t entry;
    if (read_dentry_by_name(filename, &entry) != 0) return -1;  // get the dentry of the filename we are on

    if (entry.filetype != 1) return -1;  // check file type to make sure it's a directory

    return 0;
}

/* directory_close
 * 
 * closes the opened directory
 * Inputs: fd -- file descriptor index we are closing
 * Outputs: returns 0, or -1 if fd is invalid
 * Side Effects: None
 */
int32_t directory_close(int32_t fd){
    if(fd < 0 || fd >= 8){  // check fd index
        return -1; 
    }
    return 0;
}

/* directory_write
 * 
 * does nothing, this is a read only file system
 * Inputs: fd -- file descriptor index we are writing to
 *          buf -- the buffer filled with what you want to write
 *          nbytes - amount of bytes you want to read
 * Outputs: returns -1 bc this is a read only file system
 * Side Effects: None
 */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes){
    if (buf == NULL) return -1;  // check for NULL buffer
    if (fd < 0 || fd >= 8) return -1;  // check fd index

    return -1;  // read only file system. Return -1
}

/* directory_read
 * 
 * reads one filename from the directory
 * Inputs: fd -- file descriptor index of the directory we are reading from
 *          buf -- buffer we fill in with the data we are reading
 *          nbytes -- the number of bytes we want to read
 * Outputs: returns 0, or -1 if fd is invalid
 * Side Effects: None
 */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes){
    int32_t read = 0;
    uint8_t file_char;
    dentry_t entry;
    uint8_t* buffer;

    //check buf null ptr
    if (buf == NULL) return -1;   // check for NULL buffer
    if (fd < 0 || fd >= 8) return -1;  // check fd index

    int8_t pid = pid_arr[(uint8_t)terminal_process_index];

    fda_entry_t* curr_file = &pcb_array[(uint8_t)pid]->fdarray[fd];
    
    //check to see if you reached end of directory
    if(read_dentry_by_index(curr_file->file_pos, &entry) == -1) return 0;

    buffer = (uint8_t*) buf;

    while (read < nbytes && read < FILENAME_LEN) { // loop through the bytes in the filename to fill the buffer
        file_char = entry.filename[read];

        if (file_char == '\0') break; //end of string

        buffer[read] = file_char;  // fill in the buffer with the filename

        read++;
    }

    curr_file->file_pos++;

    return read;  // return number of bytes read
}
