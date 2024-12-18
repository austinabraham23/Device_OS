#define ASM     1

#include "assembly_linkage.h"


# assembly linkage framework macro.
# func -- C handler function to link to through x86
# name -- linkage function that leads the idt table to here for assembly linkage

.macro CREATE_HANDLER name, func
.global \name             # .global \name           
\name:                 # \name:                          
    pushl %eax                      
    pushl %ebp                      
    pushl %edi                      
    pushl %esi        # push all registers              
    pushl %edx                      
    pushl %ecx                     
    pushl %ebx
    pushfl                      
    call \func        # call C function to link to
    popfl             
    popl %ebx                       
    popl %ecx                       
    popl %edx                       
    popl %esi                       
    popl %edi          # pop all registers           
    popl %ebp                       
    popl %eax                       
    iret                            
.endm

CREATE_HANDLER keyboard_handler_linkage, keyboard_handler   # enable assembly linkage for keyboard handler
CREATE_HANDLER rtc_handler_linkage, rtc_handler      # enable assembly linkage for rtc handler
CREATE_HANDLER pit_handler_linkage, pit_handler      # enable assmbly linkage for pit handler
CREATE_HANDLER mouse_handler_linkage, mouse_handler

.global system_call_linkage
# assembly linkage for system calls
system_call_linkage:

    cmpl $1, %eax
    jl fail
    cmpl $10, %eax
    jg fail

    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx

    call *jumptable(, %eax, 4)

    jmp system_call_done

fail:
    orl $0xFFFFFFFF, %eax
    iret

system_call_done:
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp

    iret  
jumptable:
    .long 0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn


