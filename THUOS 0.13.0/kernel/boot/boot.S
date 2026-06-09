/* THUOS — Multiboot boot stub.
 * Declares the Multiboot header so GRUB (or any Multiboot loader) can start
 * THUOS, sets up a stack, and calls kernel_main(magic, info). */

.set MB_ALIGN,    1 << 0              /* align loaded modules on page boundary */
.set MB_MEMINFO,  1 << 1              /* provide a memory map                  */
.set MB_FLAGS,    MB_ALIGN | MB_MEMINFO
.set MB_MAGIC,    0x1BADB002          /* Multiboot 1 magic                     */
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)

.section .multiboot
.align 4
    .long MB_MAGIC
    .long MB_FLAGS
    .long MB_CHECKSUM

.section .bss
.align 16
stack_bottom:
    .skip 16384                       /* 16 KiB kernel stack */
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    movl    $stack_top, %esp          /* establish the stack */

    pushl   %ebx                      /* arg 2: Multiboot info pointer */
    pushl   %eax                      /* arg 1: Multiboot magic        */
    call    kernel_main

    cli
.hang:
    hlt
    jmp     .hang

.size _start, . - _start

.section .note.GNU-stack,"",@progbits
