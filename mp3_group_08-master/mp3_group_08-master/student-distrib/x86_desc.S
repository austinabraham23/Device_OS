# x86_desc.S - Set up x86 segment descriptors, descriptor tables
# vim:ts=4 noexpandtab

#define ASM     1
#include "x86_desc.h"

.text

.globl ldt_size, tss_size
.globl gdt_desc, ldt_desc, tss_desc
.globl tss, tss_desc_ptr, ldt, ldt_desc_ptr
.globl gdt_ptr
.globl idt_desc_ptr, idt
.globl gdt_desc_ptr

.align 4

/* Enabling 

    - all needed is to load CR3 with the address of the page directory and to set paging (PG) and protection (PE) bits to CR0 
    - setting the paging flag when the protection flag is clear causes a general protection exception
    - once paging has been enabled, any attempt to enable long mode by setting LME of the EFER register
      will trigger a GPF (general protection flag)
        - CR0.PG must first be cleared before EFER.LME can be set
    - if you want to set pages as read-only for both userspace and supervisor, replace  0x80000001 above with 0x80010001,
      which also sets the WP bit.
*/

.globl flush_tlb
flush_tlb:
    PUSHL %ebp
    MOVL %esp, %ebp
    MOVL %cr3, %eax
    MOVL %eax, %cr3
    MOVL %ebp, %esp
    POPL %ebp
    ret

.text
.globl load_page_directory
load_page_directory:
    # load CR3 with the address of the page directory
    PUSHL %ebp
    MOVL %esp, %ebp
    MOVL 8(%ebp), %eax
    MOVL %eax, %cr3
    MOVL %ebp, %esp
    POPL %ebp
    ret

.globl enable_paging
enable_paging:
    # set values to cr4 & cr0
    PUSHL %ebp
    MOVL %esp, %ebp

    MOVL %cr4, %eax
    ORL $0x00000010, %eax
    MOVL %eax, %cr4

    MOVL %cr0, %eax
    ORL $0x80000001, %eax
    MOVL %eax, %cr0

    leave
    ret

     






tss_size:
    .long tss_bottom - tss - 1

ldt_size:
    .long ldt_bottom - ldt - 1

    .word 0 # Padding
ldt_desc:
    .word KERNEL_LDT
    .long ldt

    .align 4
tss:
_tss:
    .rept 104
    .byte 0
    .endr
tss_bottom:
    .align  16

    .align 4
    .word 0 # Padding
gdt_desc_ptr:
    .word gdt_bottom - gdt - 1
    .long gdt
    .align 16
gdt:
_gdt:

    # First GDT entry cannot be used
    .quad 0

    # NULL entry
    .quad 0

    # Segmentation will not be used
    # CS and DS both are 0-4GB r/w segments
    #
    # The layout is (from Intel IA-32 reference manual):
    #  31        24 23  22  21  20  19   16 15  14 13 12  11   8 7          0
    # |----------------------------------------------------------------------|
    # |            |   | D |   | A |  Seg  |   |  D  |   |      |            |
    # | Base 31:24 | G | / | 0 | V | Limit | P |  P  | S | Type | Base 23:16 |
    # |            |   | B |   | L | 19:16 |   |  L  |   |      |            |
    # |----------------------------------------------------------------------|
    #
    # |----------------------------------------------------------------------|
    # |                                    |                                 |
    # | Base 15:0                          | Segment Limit 15:0              |
    # |                                    |                                 |
    # |----------------------------------------------------------------------|

gdt_ptr:
    # Set up an entry for kernel CS
    .quad 0x00CF9A000000FFFF

    # Set up an entry for kernel DS
    .quad 0x00CF92000000FFFF

    # Set up an entry for user CS
    .quad 0x00CFFA000000FFFF

    # Set up an entry for user DS
    .quad 0x00CFF2000000FFFF

    # Set up an entry for TSS
tss_desc_ptr:
    .quad 0

    # Set up one LDT
ldt_desc_ptr:
    .quad 0

gdt_bottom:

    .align 16
ldt:
    .rept 4
    .quad 0
    .endr
ldt_bottom:




.align 4
    .word 0 # Padding
idt_desc_ptr:
    .word idt_bottom - idt - 1
    .long idt


    .align  16
idt:
_idt:   # make a strcut that corresponds to idt entry, then make an array that is filled with those structs, thatll be my idt table, Then point something to the idt table?--->  lidt idt_desc_table
    .rept NUM_VEC
    .quad 0
    .endr

idt_bottom:
