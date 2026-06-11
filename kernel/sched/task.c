/* THUOS — kernel task stack setup (pure; no asm, no kernel deps -> host-tested).
 *
 * The initial stack is laid out so that thuos_context_switch's restore sequence
 * (pop ebp; pop edi; pop esi; pop ebx; ret) lands in entry() with a clean stack:
 *
 *   higher addr  [ entry ]   <- 'ret' jumps here
 *                [  0    ]   ebx
 *                [  0    ]   esi
 *                [  0    ]   edi
 *   esp ->       [  0    ]   ebp
 */
#include "task.h"

#define STACK_ALIGN 16u

uint32_t task_init(ktask_t *t, uint8_t *stack, uint32_t size, void (*entry)(void)) {
    t->stack = stack;
    t->stack_sz = size;

    uintptr_t top = (uintptr_t)stack + size;
    top &= ~(uintptr_t)(STACK_ALIGN - 1);        /* align the top down */
    uint32_t *sp = (uint32_t *)top;

    *--sp = (uint32_t)(uintptr_t)entry;          /* return address -> entry() */
    *--sp = 0;                                   /* saved ebx */
    *--sp = 0;                                   /* saved esi */
    *--sp = 0;                                   /* saved edi */
    *--sp = 0;                                   /* saved ebp (esp points here) */

    t->esp = (uint32_t)(uintptr_t)sp;
    return t->esp;
}
