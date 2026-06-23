/* THUOS — minimal kernel task (execution context) for cooperative switching.
 * task_init sets up a fresh task's stack so the first switch-in begins running
 * entry() on its own stack; thuos_context_switch (context.S) does the actual
 * register/stack swap. The stack-frame setup is pure and host-tested
 * (tests/test_task.c); the asm switch is boot-verified in QEMU by CI. */
#ifndef THUOS_TASK_H
#define THUOS_TASK_H

#ifdef THUOS_HOSTED_TEST
#include <stdint.h>
#include <stddef.h>
#else
#include "types.h"
#endif

typedef struct ktask {
    uint32_t esp;        /* saved kernel stack pointer */
    uint8_t *stack;      /* base of this task's stack  */
    uint32_t stack_sz;   /* stack size in bytes        */
} ktask_t;

/* Build the initial stack frame; returns (and stores) the initial esp. */
uint32_t task_init(ktask_t *t, uint8_t *stack, uint32_t size, void (*entry)(void));

/* asm (context.S): save current context to *save_old_esp, switch to new_esp. */
extern void thuos_context_switch(uint32_t *save_old_esp, uint32_t new_esp);

#endif /* THUOS_TASK_H */
