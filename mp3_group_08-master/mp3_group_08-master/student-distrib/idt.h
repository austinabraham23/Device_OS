/* x86_desc.h - Defines for various x86 descriptors, descriptor tables,
 * and selectors
 * vim:ts=4 noexpandtab
 */

#ifndef _IDT_H
#define _IDT_H

#include "x86_desc.h"
#include "assembly_linkage.h"
#include "lib.h"

void int_idt(); //idt initialization function, fills idt table

//extern void keyboard_handler_linkage();

#endif /* _IDT_H */
