/* THUOS — int 0x80 entry stub. Builds the same registers_t frame the ISR/IRQ
 * stubs use, calls syscall_dispatch(registers_t*), then returns; the dispatcher
 * writes the result into the saved eax, which popa restores for the caller.
 * Works from ring 0 now and from ring 3 later (same DPL-3 gate). */
.text
.global syscall_stub
.type syscall_stub, @function
.extern syscall_dispatch
syscall_stub:
    cli
    pushl   $0                /* dummy err_code (frame uniformity) */
    pushl   $0x80             /* int_no */
    pusha
    xorl    %eax, %eax
    movw    %ds, %ax
    pushl   %eax              /* save data segment */

    movw    $0x10, %ax        /* kernel data segment */
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs

    pushl   %esp              /* registers_t * */
    call    syscall_dispatch
    addl    $4, %esp

    popl    %eax              /* restore data segment */
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs

    popa                      /* restores eax = syscall return value */
    addl    $8, %esp          /* drop int_no and err_code */
    sti
    iret

.section .note.GNU-stack,"",@progbits
