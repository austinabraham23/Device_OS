#include "rtc.h"
#include "i8259.h"

/* rtc_init
 * 
 * initializes rtc to generate interrupts periodically
 * Inputs: None
 * Outputs: None
 * Side Effects: initializes rtc_counter to 0, calls rtc_change_rate to set rtc clock to
 * 1024 Hz, enables irq 8, and prints RTC initialized
 */

void rtc_init(){
    outb(rtc_reg_B, rtc_ioport_1);       //select register B, and disable NMI
    char prev = inb(rtc_ioport_2);  //read the current value of register B
    outb(rtc_reg_B, rtc_ioport_1);       //set the index again (a read will reset the index to a register) 
    outb(prev | 0x40, rtc_ioport_2);//write the prevous values ORed with 0x40. This turns on bit 6 of register B
    rtc_counter = 0;
    rtc_frequency = 0;
    rtc_interrupt = 0;

    rtc_change_rate(6); //1024 hz (3)

    enable_irq(8); //irq port 8 of rtc
}

/* rtc_change_rate
 * 
 * changes rtc frequency to a value specified by rate
 * frequency = 32768 >> (rate - 1)
 * Inputs: rate: specifies frquency to be set by equation ebove
 * Outputs: 0 on success
 * Side Effects: changes rate at which interrupts are generated on IRQ8
 */
int rtc_change_rate(int rate){
    rate &= 0x0F;               //rate is greather than 2 and less than 15
    cli();
    outb(rtc_reg_A, rtc_ioport_1);       //set index to register A, disable NMI
    char prev = inb(rtc_ioport_2);  //read the current value of register A
    outb(rtc_reg_A, rtc_ioport_1);       //reset index to A
    outb(((prev & 0xF0) | rate), rtc_ioport_2); //write only our rate to A. Rate is bottom 4 bits
    sti();
    return 0; //success
}

/* rtc_handler
 * 
 * handler for rtc interrupt
 * Inputs: none
 * Outputs: none
 * Side Effects: = reads and throws away contents of register C, increments rtc_counter,
 * and sends eoi signal to IRQ8
 */
void rtc_handler(){
    outb(rtc_reg_C, rtc_ioport_1);   //select register C
    inb(rtc_ioport_2);          //throw away contents
    rtc_counter++;              //increment rtc_counter
    rtc_interrupt = 1;
    send_eoi(8);                //send eoi on irq port 8 of rtc
}


/* rtc_read
 * 
 * Blocks until the next RTC interrupt occurs.
 * Inputs: uint32_t fd - not used in the function
 *         void* buf - not used in the function
 *         uint32_t nbytes - not used in the function
 * Outputs: 0 on success
 * Side Effects: Waits for an RTC interrupt by checking rtc_interrupt.
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
    rtc_interrupt = 0; // reset interrupt to 0
    while(1){
        if (rtc_interrupt == 1){ // check to see if an interrupt has occured
            return 0; // return success
        }
    }
    return -1; // return failure
}


/* rtc_write
 * 
 * Changes the RTC frequency based on the value in buf.
 * Inputs: uint32_t fd - not used in the function
 *         const void* buf - a pointer to the new frequency value
 *         uint32_t nbytes - not used in the function
 * Outputs: 0 on success, -1 if the frequency is not a power of 2
 * Side Effects: Sets global variable rtc_frequency and calls rtc_change_rate.
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
    uint32_t frequency = *((uint32_t*)buf); // grabs frequency from the buffer
    
    // ensures the frequency is in bounds
    if (frequency <= 0 || frequency > MAX_FREQ){
        return -1; // return failure
    }

    rtc_frequency = frequency; // sets the rtc_frequency to the new value

    // caculates the rate from the frequency based on [frequency = 32768 >> (rate - 1)] (32768 comes from a rate of 1)
    int rate = 0;
    int rate_divider = 32768 / frequency;
    while (rate_divider >= 1){
        rate_divider /= 2;
        rate += 1;
    }

    rtc_change_rate(rate); // calls function to change the rate of the RTC

    return 0; // return success
}


/* rtc_open
 * 
 * Initializes RTC frequency to 2Hz.
 * Inputs: const uint8_t* filename - not used in the function
 * Outputs: 0 on success
 * Side Effects: Sets global variables rtc_frequency and calls rtc_change_rate.
 */
int32_t rtc_open(const uint8_t* filename){
    rtc_frequency = 2;  // sets the rtc_frequency to 2Hz 
    rtc_change_rate(15); // rate is 15 for a frequency of 2Hz
    
    return 0; // return success
}



/* rtc_close
 * 
 * Clears global variables related to RTC.
 * Inputs: uint32_t fd - not used in the function
 * Outputs: 0 on success
 * Side Effects: Resets global variables rtc_counter and rtc_frequency.
 */
int32_t rtc_close(int32_t fd){
    // resets global variables
    rtc_counter = 0;
    rtc_frequency = 0;

    return 0; // return success
}
