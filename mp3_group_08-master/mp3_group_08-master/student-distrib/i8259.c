/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */


/* void i8259_init(void);
 * Inputs: void
 * Return Value: void
 * Function: Initialize the 8259 PIC ports with PIC commands 
 * and data for primary and secondary PIC */
void i8259_init(void) {

    master_mask = 0xFF;   // masking all for both slave and master
    slave_mask = 0xFF;

    outb(ICW1, PIC1_COMMAND);  // starts the initialization sequence (in cascade mode)

	outb(ICW1, PIC2_COMMAND);

	outb(MASTER_OFFSET, PIC1_DATA);                 // Connect the data for PIC 1 to the offset in IDT
	
	outb(SLAVE_OFFSET, PIC2_DATA);                 // Connect the data for PIC2 to the offset in IDT
	
	outb(ICW3_MASTER, PIC1_DATA);                       // tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	
	outb(ICW3_SLAVE, PIC2_DATA);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	

    outb(ICW4_8086, PIC1_DATA);               // ICW4_8086: have the PICs use 8086 mode (and not 8080 mode)
	
	outb(ICW4_8086, PIC2_DATA);
	
 
	outb(master_mask, PIC1_DATA);   // restore saved masks.
	outb(slave_mask, PIC2_DATA);

    //printf("PIC initialized \n");
    enable_irq(0x02);  //initialize slave pin on master
}

/* void enable_irq(uint32_t irq_num);
 * Inputs: irq_num -- the irq number we want to enable / unmask
 * Return Value: void
 * Function: Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {

    uint16_t port;
    uint8_t value;
 
    if(irq_num < PIC_SIZE) {
        port = PIC1_DATA;   // set port to PIC1 data if its on primary pic, and PIC2Data if its on secondary
    } else {
        port = PIC2_DATA;
        irq_num -= PIC_SIZE;
    }
    value = inb(port) & ~(1 << irq_num);  // unmask the IRQ off my ANDing with a 0 in the spot of the IRQ_num
    outb(value, port); 

}

/* void disable_irq(uint32_t irq_num);
 * Inputs: irq_num -- the irq number we want to disable / mask
 * Return Value: void
 * Disable (mask) the specified IRQ  */
void disable_irq(uint32_t irq_num) {

    uint16_t port;
    uint8_t value;
 
    if(irq_num < PIC_SIZE) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_num -= PIC_SIZE;
    }
    value = inb(port) | (1 << irq_num);    //  mask the IRQ on by ORing it with a 1 in its IRQ num spot
    outb(value, port);   

}

/* void send_eoi(uint32_t irq_num);
 * Inputs: irq_num -- the irq number we want to send the eoi signal to
 * Return Value: void
 * Send end-of-interrupt signal for the specified IRQ  */
void send_eoi(uint32_t irq_num) {

    if(irq_num >= PIC_SIZE){
        irq_num -= PIC_SIZE;
		outb(EOI | irq_num, PIC2_COMMAND);   // send EOI to PIC2 if irq num is greater than 8, meaning its a secondary PIC
        irq_num = 2;   // chagne irq num to 2 for sending eoi
    }
	outb(EOI | irq_num, PIC1_COMMAND);   // otherwise send EOI to primary PIC1
}
