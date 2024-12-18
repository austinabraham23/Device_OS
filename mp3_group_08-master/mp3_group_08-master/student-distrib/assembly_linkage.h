#ifndef _ASSEMBLY_LINKAGE_H
#define _ASSEMBLY_LINKAGE_H

#ifndef ASM

// linkage functions for rtc and keyboard
extern void keyboard_handler_linkage();
extern void rtc_handler_linkage();
extern void system_call_linkage();
extern void pit_handler_linkage();
extern void mouse_handler_linkage();

#endif /* ASM */

#endif /* _ASSEMBLY_LINKAGE_H */
