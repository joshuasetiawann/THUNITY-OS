/* THUOS — Task State Segment.
 * In 32-bit protected mode the TSS is not used for hardware task switching here;
 * its job is to hold the kernel stack (SS0:ESP0) the CPU loads automatically
 * whenever execution crosses from ring 3 back into ring 0 (a syscall via
 * int 0x80, or a hardware IRQ taken while in user mode). Without a loaded TSS,
 * that privilege transition has nowhere to put its stack and faults. */
#ifndef THUOS_TSS_H
#define THUOS_TSS_H

#include "types.h"

void     tss_init(void);              /* zero the TSS, set SS0/ESP0/iomap     */
void     tss_set_esp0(uint32_t esp0); /* kernel stack used on ring 3 -> ring 0 */
uint32_t tss_base(void);              /* &g_tss  (for the GDT descriptor)      */
uint32_t tss_limit(void);             /* sizeof(g_tss) - 1                     */
uint16_t tss_selector(void);          /* GDT selector for the TSS (0x28)       */

/* usermode.S: load the task register (ltr) with the given selector. */
extern void tss_flush(uint32_t selector);

#endif /* THUOS_TSS_H */
