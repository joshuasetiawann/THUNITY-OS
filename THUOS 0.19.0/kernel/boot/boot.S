/* THUOS — Multiboot boot stub.
 * Declares the Multiboot header so GRUB (or any Multiboot loader) can start
 * THUOS, sets up a stack, and calls kernel_main(magic, info). */

.set MB_ALIGN,    1 << 0              /* align loaded modules on page boundary */
.set MB_MEMINFO,  1 << 1              /* provide a memory map                  */
.set MB_VIDEO,    1 << 2              /* ask the loader for a linear framebuffer */
.set MB_FLAGS,    MB_ALIGN | MB_MEMINFO | MB_VIDEO
.set MB_MAGIC,    0x1BADB002          /* Multiboot 1 magic                     */
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)

/* Multiboot 1 header. Requesting MB_VIDEO makes a real bootloader (GRUB on
 * BIOS via VBE, or GRUB-EFI via UEFI GOP) set a linear framebuffer for us and
 * report its address/pitch/format in the info struct — the portable way to get
 * graphics on real Intel/AMD hardware, which has no Bochs VBE device. The fields
 * sit at fixed offsets: the five a.out-kludge longs (unused, since flag bit 16
 * is clear) must be present so mode_type lands at offset 32. */
.section .multiboot
.align 4
    .long MB_MAGIC
    .long MB_FLAGS
    .long MB_CHECKSUM
    .long 0, 0, 0, 0, 0               /* header_addr..entry_addr (a.out kludge, unused) */
    .long 0                           /* mode_type: 0 = linear graphics framebuffer */
    .long 1024                        /* preferred width  */
    .long 768                         /* preferred height */
    .long 32                          /* preferred depth (bits per pixel) */

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
