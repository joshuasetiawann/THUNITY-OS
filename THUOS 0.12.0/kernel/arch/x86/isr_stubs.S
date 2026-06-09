/* THUOS — assembly entry stubs for CPU exceptions (0-31) and IRQs (32-47).
 * Each stub normalises the stack into a `registers_t` frame, then calls the
 * C dispatcher. AT&T / GAS syntax. */
.text

/* Exceptions that do NOT push an error code: push a dummy 0 for uniformity. */
.macro ISR_NOERR num
.global isr\num
.type isr\num, @function
isr\num:
    cli
    pushl $0
    pushl $\num
    jmp   isr_common_stub
.endm

/* Exceptions that DO push an error code (8, 10-14). */
.macro ISR_ERR num
.global isr\num
.type isr\num, @function
isr\num:
    cli
    pushl $\num
    jmp   isr_common_stub
.endm

.macro IRQ num, vector
.global irq\num
.type irq\num, @function
irq\num:
    cli
    pushl $0
    pushl $\vector
    jmp   irq_common_stub
.endm

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

.extern isr_handler
.extern irq_handler

isr_common_stub:
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
    call    isr_handler
    addl    $4, %esp

    popl    %eax              /* restore data segment */
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs

    popa
    addl    $8, %esp          /* drop int_no and err_code */
    sti
    iret

irq_common_stub:
    pusha
    xorl    %eax, %eax
    movw    %ds, %ax
    pushl   %eax

    movw    $0x10, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs

    pushl   %esp
    call    irq_handler
    addl    $4, %esp

    popl    %eax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs

    popa
    addl    $8, %esp
    sti
    iret

.section .note.GNU-stack,"",@progbits
