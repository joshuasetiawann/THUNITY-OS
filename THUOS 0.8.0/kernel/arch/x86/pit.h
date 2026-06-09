/* THUOS — Programmable Interval Timer (8253/8254) on IRQ0. */
#ifndef THUOS_PIT_H
#define THUOS_PIT_H

#include "types.h"

void     pit_init(uint32_t frequency_hz);
uint32_t pit_ticks(void);
uint32_t pit_frequency(void);
uint32_t pit_seconds(void);

#endif /* THUOS_PIT_H */
