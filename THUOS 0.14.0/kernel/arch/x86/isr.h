/* THUOS — interrupt service routines: CPU exceptions (0-31) and the
 * register frame shared by the assembly stubs. */
#ifndef THUOS_ISR_H
#define THUOS_ISR_H

#include "types.h"

/* Pushed by the common assembly stub. Field order matches the stack layout. */
typedef struct {
    uint32_t ds;                                      /* saved data segment   */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha                 */
    uint32_t int_no, err_code;                        /* pushed by stub       */
    uint32_t eip, cs, eflags, useresp, ss;            /* pushed by CPU         */
} registers_t;

typedef void (*isr_handler_t)(registers_t *);

void isr_init(void);
void isr_register_handler(uint8_t n, isr_handler_t handler);

#endif /* THUOS_ISR_H */
