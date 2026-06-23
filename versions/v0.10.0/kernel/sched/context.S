/* THUOS — cooperative context switch (i386).
 *
 * void thuos_context_switch(uint32_t *save_old_esp, uint32_t new_esp);
 *
 * Saves the callee-saved registers of the current context onto its stack,
 * stores the resulting esp into *save_old_esp, switches to new_esp, restores
 * that context's callee-saved registers, and returns into it. cdecl: arg1 at
 * 4(%esp), arg2 at 8(%esp) on entry. */
.section .text
.global thuos_context_switch
.type thuos_context_switch, @function
thuos_context_switch:
    movl 4(%esp), %eax        /* eax = save_old_esp (pointer) */
    movl 8(%esp), %edx        /* edx = new_esp (value)        */

    pushl %ebx                /* save callee-saved regs       */
    pushl %esi
    pushl %edi
    pushl %ebp

    movl %esp, (%eax)         /* *save_old_esp = current esp  */
    movl %edx, %esp           /* load the new stack           */

    popl %ebp                 /* restore callee-saved regs    */
    popl %edi
    popl %esi
    popl %ebx
    ret                       /* return into the new context  */
.size thuos_context_switch, . - thuos_context_switch

.section .note.GNU-stack,"",@progbits
