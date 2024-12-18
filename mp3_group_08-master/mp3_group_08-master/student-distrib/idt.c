#include "idt.h"

// Local Function Declarations
static void exception_handler();
static void divide_zero_exception();
static void debug_exception();
static void nmi_interrupt();
static void breakpoint_exception();
static void overflow_exception();
static void bound_range_exceeded_exception();
static void invalid_opcode_exception();
static void device_not_available_exception();
static void double_fault_exception();
static void coprocessor_segment_overrun_exception();
static void invalid_tss_exception();
static void segment_not_present_exception();
static void stack_fault_exception();
static void general_protection_exception();
static void page_fault_exception();
// EXCEPTION 15 RESERVED
static void x87_floating_point_exception();
static void alignment_check_exception();
static void machine_check_exception();
static void simd_floating_point_exception();


/* int_idt
 * 
 * fills idt table with exception handlers, rtc, and kayboard
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 */
void int_idt(){
    int i;

    // initialize the entries of the idt table
    for(i = 0; i < 256; i++){ 

        idt[i].seg_selector = KERNEL_CS;  
        idt[i].reserved4 = 0x00;
        if(i < 32 || i == 128){  // if its an exception handler  or system call reserved bit 3 should be 1 for the TRAP gate bits
            idt[i].reserved3 = 0x1;
        }else{
            idt[i].reserved3 = 0x0;
        }
        idt[i].reserved2 = 0x1;
        idt[i].reserved1 = 0x1;
        idt[i].size = 0x01;  
        idt[i].reserved0 = 0x0;   
        if(i==128){
            idt[i].dpl = 0x3;   // dpl should only be 3 for system call
        } else{
            idt[i].dpl = 0x0;
        }
        idt[i].present = 0x0;


    }

    SET_IDT_ENTRY(idt[0x00], divide_zero_exception);
    SET_IDT_ENTRY(idt[0x01], debug_exception);
    SET_IDT_ENTRY(idt[0x02], nmi_interrupt);
    SET_IDT_ENTRY(idt[0x03], breakpoint_exception);
    SET_IDT_ENTRY(idt[0x04], overflow_exception);

    SET_IDT_ENTRY(idt[0x05], bound_range_exceeded_exception);
    SET_IDT_ENTRY(idt[0x06], invalid_opcode_exception);
    SET_IDT_ENTRY(idt[0x07], device_not_available_exception);
    SET_IDT_ENTRY(idt[0x08], double_fault_exception);
    SET_IDT_ENTRY(idt[0x09], coprocessor_segment_overrun_exception);

    SET_IDT_ENTRY(idt[0x0A], invalid_tss_exception);
    SET_IDT_ENTRY(idt[0x0B], segment_not_present_exception);
    SET_IDT_ENTRY(idt[0x0C], stack_fault_exception);
    SET_IDT_ENTRY(idt[0x0D], general_protection_exception);
    SET_IDT_ENTRY(idt[0x0E], page_fault_exception);

    SET_IDT_ENTRY(idt[0x0F], exception_handler);
    SET_IDT_ENTRY(idt[0x10], x87_floating_point_exception);
    SET_IDT_ENTRY(idt[0x11], alignment_check_exception);
    SET_IDT_ENTRY(idt[0x12], machine_check_exception);
    SET_IDT_ENTRY(idt[0x13], simd_floating_point_exception);

    SET_IDT_ENTRY(idt[0x20], pit_handler_linkage);
    SET_IDT_ENTRY(idt[0x21], keyboard_handler_linkage);    // need assembly linkage
    SET_IDT_ENTRY(idt[0x28], rtc_handler_linkage);    // need assembly linkage
    SET_IDT_ENTRY(idt[0x2C], mouse_handler_linkage);

    SET_IDT_ENTRY(idt[0x80], system_call_linkage);  // asm linkage

    

    // also need to implement system call later


}


/* exception_handler
 * 
 * general exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints exception handler, loops infinitely 
 */
static void exception_handler(){
    printf("exception handler \n");
    while(1);
    return;
}

/* divide_zero_exception
 * 
 * divide zero exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints divide zero exception, loops infinitely 
 */
static void divide_zero_exception(){  // exception 0
    printf("Divide Zero Exception\n");
    while(1);
    return;
}

/* divide_zero_exception
 * 
 * debug exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints debug exception, loops infinitely 
 */
static void debug_exception(){  // exception 1
    printf("Debug Exception \n");
    while(1);
    return;
}

/* nmi_interrupt
 * 
 * nmi interrupt handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints NMI interrupt, loops infinitely 
 */
static void nmi_interrupt(){  // exception 2
    printf("NMI Interrupt \n");
    while(1);
    return;
}

/* breakpoint_exception
 * 
 * breakpoint exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints breakpoint exception, loops infinitely 
 */
static void breakpoint_exception(){  // exception 3
    printf("Breakpoint Exception \n");
    while(1);
    return;
}

/* overflow_exception
 * 
 * overflow exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints overflow exception, loops infinitely 
 */
static void overflow_exception(){  // exception 4
    printf("Overflow Exception \n");
    while(1);
    return;
}

/* bound_range_exceeded_exception
 * 
 * bound range exceeded exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints bound range exceeded exception, loops infinitely 
 */
static void bound_range_exceeded_exception(){  // exception 5
    printf("Bound Range Exceeded Exception \n");
    while(1);
    return;
}

/* invalid_opcode_exception
 * 
 * invalid opcode exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints invalid opcode exception, loops infinitely 
 */
static void invalid_opcode_exception(){  // exception 6
    printf("Invalid Opcode Exception \n");
    while(1);
    return;
}

/* device_not_available_exception
 * 
 * device not available exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints device not available exception, loops infinitely 
 */
static void device_not_available_exception(){  // exception 7
    printf("Device Not Available Exception \n");
    while(1);
    return;
}

/* double_fault_exception
 * 
 * double fault exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints double fault exception, loops infinitely 
 */
static void double_fault_exception(){  // exception 8
    printf("Double Fault Exception \n");
    while(1);
    return;
}

/* coprocessor_segment-overrun_exception
 * 
 * coprocessor segment overrun exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints coprocessor segment overrun exception, loops infinitely 
 */
static void coprocessor_segment_overrun_exception(){  // exception 9
    printf("Coprocessor Segment Overrun Exception \n");
    while(1);
    return;
}

/* invalid_tss_exception
 * 
 * invalid tss exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints invalid tss exception, loops infinitely 
 */
static void invalid_tss_exception(){  // exception 10
    printf("Invalid TSS Exception \n");
    while(1);
    return;
}

/* segment_not_present_exception
 * 
 * segment not present exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints segment not present exception, loops infinitely 
 */
static void segment_not_present_exception(){  // exception 11
    printf("Segment Not Present Exception \n");
    while(1);
    return;
}

/* stack_fault_exception
 * 
 * stack fault exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints stack fault exception, loops infinitely 
 */
static void stack_fault_exception(){  // exception 12
    printf("Stack Fault Exception \n");
    while(1);
    return;
}

/* general_protection_exception
 * 
 * general protection exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints general protection exception, loops infinitely 
 */
static void general_protection_exception(){  // exception 13
    printf("General Protection Exception \n");
    while(1);
    return;
}

/* page_fault_exception
 * 
 * page fault exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints page fault exception, loops infinitely 
 */
static void page_fault_exception(){  // exception 14
    printf("Page Fault Exception \n");
    while(1);
    return;
}

// EXCEPTION 15 RESERVED

/* x87_floating_point_exception
 * 
 * x87 floating point exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints x87 FPU floating point error exception, loops infinitely 
 */
static void x87_floating_point_exception(){  // exception 16
    printf("x87 FPU Floating Point Error Exception \n");
    while(1);
    return;
}

/* alignment_check_exception
 * 
 * alignment check exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints alignment check exception, loops infinitely 
 */
static void alignment_check_exception(){  // exception 17
    printf("Alignment Check Exception \n");
    while(1);
    return;
}

/* machine_check_exception
 * 
 * machine check exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints machine check exception, loops infinitely 
 */
static void machine_check_exception(){  // exception 18
    printf("Machine Check Exception \n");
    while(1);
    return;
}

/* simd_floating_point_exception
 * 
 * simd floating point exception handler
 * Inputs: None
 * Outputs: None
 * Side Effects: prints simd floating point exception, loops infinitely 
 */
static void simd_floating_point_exception(){  // exception 19
    printf("SIMD Floating Point Exception \n");
    while(1);
    return;
}
