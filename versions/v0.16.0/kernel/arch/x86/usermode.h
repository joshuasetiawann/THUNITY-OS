/* THUOS — user mode (ring 3).
 * Milestone 0.12: the kernel can drop the CPU to privilege level 3, run code
 * there, and have that code re-enter the kernel only through the int 0x80
 * syscall gate, then return cleanly to ring 0. This proves the privilege
 * mechanism (GDT user segments + TSS + iret + syscall-from-userspace).
 *
 * Honesty: the address space is still the single flat identity map (now marked
 * user-accessible), so this is NOT per-process memory isolation — that is a
 * later milestone. What is demonstrated and boot-verified is the CPL 0->3->0
 * transition and a real syscall issued from ring 3. */
#ifndef THUOS_USERMODE_H
#define THUOS_USERMODE_H

#include "types.h"

void usermode_init(void);      /* register SYS_EXIT (the ring-3 return path)   */
int  usermode_run(void);       /* enter ring 3, run the demo task, return code */
void usermode_selftest(void);  /* run it once at boot and print a CPL-3 proof  */

#endif /* THUOS_USERMODE_H */
