/* THUOS — load GDTR and reload segment selectors (AT&T / GAS syntax). */
.text
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    movl    4(%esp), %eax      /* arg: &gdt_ptr */
    lgdt    (%eax)

    movw    $0x10, %ax         /* 0x10 = kernel data segment */
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movw    %ax, %ss
    ljmp    $0x08, $.flush_cs   /* 0x08 = kernel code segment */
.flush_cs:
    ret

.section .note.GNU-stack,"",@progbits
