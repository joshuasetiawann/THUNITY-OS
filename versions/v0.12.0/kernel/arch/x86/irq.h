/* THUOS — hardware IRQ routing (interrupts 32-47). */
#ifndef THUOS_IRQ_H
#define THUOS_IRQ_H

#include "types.h"
#include "isr.h"

void irq_init(void);
void irq_register_handler(uint8_t irq, isr_handler_t handler);

#endif /* THUOS_IRQ_H */
