#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "file_system.h"
#include "keyboard.h"
#include "terminal.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
 
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 20; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* Exception Test
 * 
 * Tests for exception divide by 0
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: IDT intel specification 0
 * Files: idt.c/h
 */
int exception_test(){
	TEST_HEADER;

	int j = 2;
	int i = 0;
	i = j/i;

	int result = PASS;
	printf("success for test");
	return result;
}

/* Keyboard Test
 * 
 * Allows for keys to be typed
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: PIC, Assembly Linkage, Keyboard interrupt
 * Files: keyboard.c/h
 */
int keyboard_test(){
	TEST_HEADER;
	
	while(1);
	return PASS;
}

/* RTC Test
 * 
 * Calls test_interrupts every second
 * Inputs: None
 * Outputs: PASS
 * Side Effects: Changes screen values
 * Coverage: Slave PIC, ASM Linkage
 * Files: rtc.c/h
 */
int rtc_test(){
	TEST_HEADER;
	while(1){
		if(rtc_counter%1024 == 0){
        	test_interrupts();
    	}
	}
}

/* Paging Test
 * 
 * Chooses video memory locations and dereferences, then prints to screen.
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Prints messages to screen then raises exception.
 * Coverage: Page Table initialization and enabling. Checks all of VM and the byte right after.
 * Files: paging.c/h
 */
int paging_test(){
	TEST_HEADER;
	int i;
	uint8_t* video_mem_ptr;
	uint8_t* unallocated_mem_ptr;
	uint8_t video_mem, unallocated_mem;

	i = VMEM;

	printf("Video memory dereferencing starting at %u \n", i);

	//loop through video mem only and dereference it all.
	for ( ; i < VMEM + PAGE_SIZE; i++) {			//video mem is 4kb from 184 offset, until 185 which is next page
		video_mem_ptr = (uint8_t*) i;

		//dereference a byte
		video_mem = (*video_mem_ptr);
	}

	printf("SUCCESS: Video memory dereferencing. Last dereferenced %u \n", video_mem);

	//choose a byte just outside of the VM
	unallocated_mem_ptr = (uint8_t*) (VMEM + PAGE_SIZE);

	printf("Attempting to dereference unallocated memory at addr %u \n", unallocated_mem_ptr);

	//dereference should fail
	unallocated_mem = (*unallocated_mem_ptr);

	printf("FAIL: This should not print and should fail %u \n", unallocated_mem);


	return FAIL;
}

/* Paging Before Video Memory
 * 
 * Chooses memory location before video memory and dereferences.
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Prints messages to screen then raises exception.
 * Coverage: Page Table initialization and enabling. Checks byte before VM.
 * Files: paging.c/h
 */
int paging_before_vm() {
	TEST_HEADER;

	uint8_t* unallocated_mem_ptr;
	uint8_t unallocated_mem;

	unallocated_mem_ptr = (uint8_t*) ((184 << 12) - 1);
	printf("Attempting to dereference unallocated memory at addr %u \n", unallocated_mem_ptr);

	unallocated_mem = (*unallocated_mem_ptr);

	printf("FAIL: This should not print and should fail %u \n", unallocated_mem);

	return FAIL;
}


/* Checkpoint 2 tests */

/* File System Directory ls/read test
 * 
 * Runs the directory read function to read and print the names and
 * other info about all of the files in a directory
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: directory_open, directory_read
 * Files: file_system.c/h
 */
int directory_ls() {
	TEST_HEADER;
	clear_screen();

	int32_t fd, cnt, ret;
	int32_t i = 0;
	int8_t file_name[] = "file_name: ";
	int8_t file_size[] = ", file_size:";  // initialize strings to print
	int8_t file_type[] = ", file_type: ";
	int8_t filetype;
	int32_t length;
	int8_t length_buf[10];
	int8_t spaces[33];
	int8_t buf[BUFSIZE];
	uint8_t filename[2] = {'.', '\0'};
	uint8_t indent[2] = {'\n', '\n'};
	dentry_t entry;

	memset(spaces, ' ', 33);  // set spaces to print in memory

	if (-1 == (fd = directory_open(filename))) {  // open the directory 
		printf("Directory open failed! \n");
		return FAIL;
	}

	if (-1 == (fd = terminal_open(NULL))) {
		printf("Terminal open failed! \n");  //open the terminal
		return FAIL;
	}

	while (0 != (cnt = directory_read(fd, buf, FILENAMESIZE))) {  // go through all of the files to print in the directory
		if (-1 == cnt) {
			printf("Directory entry read failed! \n");
			return FAIL;
		}
		buf[cnt] = '\0';

		ret = terminal_write(fd, file_name, strlen(file_name)); //print file_name:

		//get file size and type
		if (-1 == read_dentry_by_index(i, &entry)) {
			printf("Get file size / type failed! \n");
			return FAIL;
		}
		filetype = entry.filetype + 48; //map file type to ascii character
		length = get_file_size(entry.inode_num);

		//print rest of ls information
		ret = terminal_write(fd, spaces, FILENAMESIZE - cnt);
		ret = terminal_write(fd, buf, cnt);
		ret = terminal_write(fd, file_type, strlen(file_type));
		ret = terminal_write(fd, &filetype, 1);
		ret = terminal_write(fd, file_size, strlen(file_size));
		(void) itoa(length, length_buf, 10);
		ret = terminal_write(fd, spaces, 10 - strlen(length_buf));
		ret = terminal_write(fd, length_buf, strlen(length_buf));
		ret = terminal_write(fd, indent, 1);
		i++;
	}

	if (0 != directory_close(fd)) {
		return FAIL;
	}

	ret = terminal_write(fd, indent, 2);

	return PASS;
}

/* File System Read file Data Test
 * 
 * Runs the file  read function to read and print the content of a file
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: file_open, file_read
 * Files: file_system.c/h
 */
int read_file_data_test(int8_t* filename) {
	TEST_HEADER;
	clear_screen();
	int ret;

	int8_t filename_print[] = "\nfile_name: ";
	int8_t end_print[] = "\n\n";
	int32_t fd, cnt;
	int8_t data[FILEBUFSIZE];

	if (-1 == (fd = file_open((uint8_t*) filename))) { // open the file we are reading
        printf ("File open failed! \n");
        return FAIL;
    }

	while(0 != (cnt = file_read(fd, data, FILEBUFSIZE))){  // read the content of the file
		if (-1 == cnt) {
			printf("File read failed! \n");
		}
		ret = terminal_write(fd, data, cnt);
	}

	ret = terminal_write(fd, filename_print, strlen(filename_print));   // write the read information to the terminal
	ret = terminal_write(fd, filename, strlen(filename));
	ret = terminal_write(fd, end_print, strlen(end_print));

	if (file_close(fd) == -1) {
        return FAIL;
    }
	return PASS;
}

/* File System Garbage input tests
 * 
 * Inputs garbage inot the file system functiopns and 
 * checks for correct failure sequence
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: read_dentry_by_name, read_dentry_by_index, 
 * 		read_data, file_open, file_read, file_close, file_write, 
 * 		directory_open, directory_close, directory_read, 
 * 		directory_write
 * Files: file_system.c/h
 */
int file_sys_garbage_test(){
	TEST_HEADER;
	clear_screen();

	int ret;
	dentry_t* dentry;
	uint8_t buffer[FILEBUFSIZE];
	uint8_t fake_file_name[] = "fakefile.txt";


	if(read_dentry_by_name(NULL, dentry) != -1){   // check for invalid filename
		return FAIL;
	}
	if(read_dentry_by_index(3000, dentry) != -1){   // check for invalid indexc
		return FAIL;
	}
	if(read_dentry_by_index(-3, dentry) != -1){    // check for invalid index
		return FAIL;
	}
	if(read_data(4000, 5, buffer, 7) != -1){  // check for invalid inode number
		return FAIL;
	}
	if(read_data(6, 5, NULL, 7) != -1){   // check for null buffer 
		return FAIL;
	}
	if(file_open(NULL) != -1){   // check for null filename
		return FAIL;
	}
	if(file_open(fake_file_name) != -1){  // check for invalid filename
		return FAIL;
	}
	if(file_close(10) != -1){  // check for invalid fd value
		return FAIL;  
	}
	if(file_write(10, buffer, 8) != -1){  // check for invalid fd value
		return FAIL;
	}
	if(file_write(5, NULL, 8) != -1){  // check for null buffer input
		return FAIL;
	}
	if(file_read(10, buffer, 8) != -1){  // check for invalid fd value
		return FAIL;
	}
	if(file_read(5, NULL, 8) != -1){ // check for null buffer
		return FAIL;
	}
	if(directory_open(NULL) != -1){  // check for null filename
		return FAIL;
	}
	if(directory_open(fake_file_name) != -1){  // check for invalid filename
		return FAIL;
	}
	if(directory_close(10) != -1){  // check for invalid fd value
		return FAIL;
	}
	if(directory_write(10, buffer, 8) != -1){  // check for invalid fd value
		return FAIL;
	}
	if(directory_write(5, NULL, 8) != -1){  // check for null buffer input
		return FAIL;
	}
	if(directory_read(10, buffer, 8) != -1){  // check for invalid fd value
		return FAIL;
	}
	if(directory_read(5, NULL, 8) != -1){  // check for null buffer input
		return FAIL;
	}
	// if all garbage inputs returned correctly pass the test!
	int8_t print_success[] = "You passed the garbage test!!! \n";
	ret = terminal_write(0, print_success, strlen(print_success));
	return PASS;
}




/* RTC Open Test Helper
 * 
 * Changes frequency to 2Hz and draws '1' at that rate
 * Inputs: None
 * Outputs: PASS
 * Side Effects: Changes screen values
 * Coverage: Slave PIC, ASM Linkage
 * Files: rtc.c/h
 */
void rtc_open_test_helper(){

	char bar = 1; // char to be printed

    rtc_open(NULL); // param is not needed

    int temp = rtc_frequency; // adds one to the rtc_frequency in order to display the correct characters before the clear_screen_helper is called again
    int last_rtc_counter = rtc_counter; // holds value of the last rtc_counter after printing
    int delay_ms = 1; // sets delay value

	// prints (frequency) amount of chars at the set frequency
    while(temp > 0){
        // Check if rtc_counter has incremented and creates delay if so (to be seen better)
        if (rtc_counter > last_rtc_counter) {
            if (rtc_counter % delay_ms == 0) {
                printf("%d", bar); 
                temp--; // decrement temp
                last_rtc_counter = rtc_counter; // Update the last_rtc_counter
            }
        }
    }

	rtc_read(NULL, NULL, NULL);
}

/* RTC Write Test Helper
 * 
 * Changes frequency to specified value and draws '1' at that rate
 * Inputs: frequency
 * Outputs: PASS
 * Side Effects: Changes screen values
 * Coverage: Slave PIC, ASM Linkage
 * Files: rtc.c/h
 */
void rtc_write_test_helper(uint32_t frequency){
	
	const void* buf_frequency = &frequency; // creates frequency buffer for rtc_write
	char bar = 1; // char to be printed

	// checks if garbage value
	int return_val = rtc_write(NULL, buf_frequency, NULL);
	if (return_val == -1){
		printf("Value is not within bounds");
		rtc_read(NULL, NULL, NULL);
		return;
	}

	int temp = rtc_frequency; // adds one to the rtc_frequency in order to display the correct characters before the clear_screen_helper is called again
    int last_rtc_counter = rtc_counter; // holds value of the last rtc_counter after printing
    int delay_ms = 1; // sets delay value

	// prints (frequency) amount of chars at the set frequency
    while(temp > 0){
        // Check if rtc_counter has incremented and creates delay if so (to be seen better)
        if (rtc_counter > last_rtc_counter) {
            if (rtc_counter % delay_ms == 0) {
                printf("%d", bar);
                temp--; // decrement temp
                last_rtc_counter = rtc_counter; // Update the last_rtc_counter
            }
        }
    }

	rtc_read(NULL, NULL, NULL);
}

/* RTC Open/Write Test
 * 
 * Changes frequency to specified values and draws '1' at that rate
 * Inputs: NONE
 * Outputs: PASS
 * Side Effects: Changes screen values
 * Coverage: Slave PIC, ASM Linkage
 * Files: rtc.c/h
 */
int rtc_open_write_test(){
	int i;
	
	// creates open
	clear_screen();
	rtc_open_test_helper();
	
	// tests writing at different frequencies from 4 up to 1028
	for (i = 4; i <= 1024; i = i*2){
		clear_screen();
		rtc_write_test_helper(i);
	}

	// tests garbage inputs of frequency for rtc_write
	clear_screen();
	printf("NOW TESTING GARBAGE INPUTS \n");
	rtc_write_test_helper(349210432); //garbage frequency
	printf("\n");
	rtc_write_test_helper(-129323); //garbage frequency
	printf("\n");
	
	return PASS;
}


/* RTC Read Test
 * 
 * Changes frequency to specified value, calls test_interrupts every second,
 * 		and blocks until the next RTC interrupt occurs
 * Inputs: None
 * Outputs: PASS
 * Side Effects: Changes screen values
 * Coverage: Slave PIC, ASM Linkage
 * Files: rtc.c/h
 */
int rtc_read_test(){
	TEST_HEADER;

	// check to see if it waits until it recieves an interrupt

	uint32_t frequency = 1024; // sets frequency to 1024 (could be any value)

	const void* buf_w = &frequency; // creates frequency buffer for rtc_write

	rtc_frequency = (unsigned)frequency; // sets rtc_frequency

	rtc_write(NULL, buf_w, NULL);
	int value = rtc_read(NULL, NULL, NULL); // holds return value of rtc_read

	clear_screen(); // clears screen for test results

	// checks return val of rtc_read
	if(value == 0){
		printf("SUCCESS: Returned Success! \n");
		return PASS;
	} else if (value == -1) {
		printf("Failure: Returned Fail! \n");
	} else {
		printf("WTFFFFF: Something is so wrong \n");
	}

	return FAIL;
}


/* terminal_test
 * 
 * A test function for terminal functionality.
 * It clears the screen and continuously reads from the terminal, then writes what's read back to the terminal.
 * Inputs: None
 * Outputs: Returns FAIL if it ever exits the infinite loop.
 * Side Effects: Clears the screen, reads from the terminal, and writes to the terminal.
 */
int terminal_test(){
	TEST_HEADER;
	clear_screen();
	while(1){
		char retbuf[128];
		memset(retbuf, 0, sizeof(retbuf));
		terminal_read(0, retbuf, sizeof(retbuf));
		terminal_write(0, retbuf, sizeof(retbuf));
	}
	return FAIL;
}


/* garbage_terminal_test
 * 
 * A test function for terminal functionality with NULL pointers and zero length.
 * It calls `terminal_read` and `terminal_write` with NULL pointers and zero length, expecting them to handle these cases.
 * Inputs: None
 * Outputs: Returns PASS if the tests pass successfully.
 * Side Effects: Calls `terminal_read` and `terminal_write` with NULL pointers and zero length.
 */
int garbage_terminal_test(){
	TEST_HEADER;
	terminal_read(0, NULL, 0);
	terminal_write(0, NULL, 0);
	return PASS;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	
	/* Checkpoint 1 tests */

	//TEST_OUTPUT("idt_test", idt_test()); // need to do this later

	//TEST_OUTPUT("exceptions test", exception_test());

	//TEST_OUTPUT("keyboard test", keyboard_test());

	//TEST_OUTPUT("paging test", paging_test());
	//TEST_OUTPUT("paging before vm", paging_before_vm());


	/* Checkpoint 2 tests */

	//TEST_OUTPUT("rtc_open_write_test", rtc_open_write_test());
	//TEST_OUTPUT("rtc_read_test", rtc_read_test());

	//TEST_OUTPUT("file ls test", directory_ls());

	//int8_t large[] = "verylargetextwithverylongname.txt";
	//TEST_OUTPUT("large file read test", read_file_data_test(large));

	// int8_t executable[] = "ls";
	// TEST_OUTPUT("exec file read test", read_file_data_test(executable));

	//int8_t frame[] = "frame0.txt";
	//TEST_OUTPUT("small file read test", read_file_data_test(frame));

	//TEST_OUTPUT("garbage input test", file_sys_garbage_test());

	//TEST_OUTPUT("terminal test", terminal_test());
	//TEST_OUTPUT("garbage terminal test", garbage_terminal_test());
}

