#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include "types.h"
#include "system_calls.h"

#define FILENAME_LEN 32
#define BLOCK_BYTE_SIZE 4096

extern int8_t pid_arr[3];

/* Holds data for the dentry of each directory */
typedef struct dentry 
{
    uint8_t filename[FILENAME_LEN];
    int32_t filetype;
    int32_t inode_num;
    int8_t reserved[24];
} dentry_t;

/* Represents the boot block in memory */
typedef struct boot_block
{
    int32_t dir_count;
    int32_t inode_count;
    int32_t data_count;
    int8_t reserved[52];
    dentry_t direntries[63];
} boot_block_t;

/* Represents inode block in memory */
typedef struct inode
{
    int32_t length;
    int32_t data_block_num[1023]; //1024 - 1 to hold length
} inode_t;

/* Represents a data block made of 4096 bytes */
typedef struct data_block
{
    int8_t byte[4096];
} data_block_t;

/* Initializes the machine for using files */
void file_sys_init(boot_block_t* boot_block_start);

/* Reads a single dentry by its file name */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);

/* Reads a single dentry by its index */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

/* Reads a number of bytes into a file */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

/* rteurns the size of the file corresponding to the inode*/
int32_t get_file_size(uint32_t inode);

/* initializes variables for current file*/
int32_t file_open(const uint8_t* filename);

/* closes the openbed file*/
int32_t file_close(int32_t fd);

/* does nothing for read only file system*/
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

/* reads nbytes of the content in the file*/
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);

/* initializes variables for the current directory*/
int32_t directory_open(const uint8_t* filename);

/* closes the opened directory */
int32_t directory_close(int32_t fd);

/* does nothing in the current read only file system */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes);

/* raeds the filenames inisde of the opened directory*/
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes);

#endif
