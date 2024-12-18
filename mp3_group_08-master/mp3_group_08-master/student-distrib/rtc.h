#ifndef _RTC_H
#define _RTC_H

#include "lib.h"

#define rtc_ioport_1 0x70 //first of two io ports used by rtc
#define rtc_ioport_2 0x71 //second of two io ports used by rtc
#define rtc_reg_A 0x8A //data to send to get rtc register A and disable NMI interrupts
#define rtc_reg_B 0x8B //data to send to get rtc register B and disable NMI interrupts
#define rtc_reg_C 0x8C //data to send to get rtc register C and disable NMI interrupts

#define MAX_FREQ    1024 // kernel limited to 1024, but max is actually 8192

// global variables
unsigned rtc_counter; //counter to check when rtc has elapsed a second
unsigned rtc_frequency; // the current frequency (used for tests)
int rtc_interrupt; // if an interrupt has occured

// initialize RTC
extern void rtc_init();
extern int rtc_change_rate(int rate);
extern void rtc_handler();

// used for system calls
extern int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t rtc_open(const uint8_t* filename);
extern int32_t rtc_close(int32_t fd);

#endif /* _RTC_H */
