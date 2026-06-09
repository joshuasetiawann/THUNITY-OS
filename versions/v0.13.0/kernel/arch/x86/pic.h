/* THUOS — 8259 Programmable Interrupt Controller. */
#ifndef THUOS_PIC_H
#define THUOS_PIC_H

#include "types.h"

#define PIC1_OFFSET 0x20  /* IRQ 0-7  -> interrupts 32-39 */
#define PIC2_OFFSET 0x28  /* IRQ 8-15 -> interrupts 40-47 */

void pic_remap(void);
void pic_send_eoi(uint8_t irq);

#endif /* THUOS_PIC_H */
