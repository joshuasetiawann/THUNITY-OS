# 02 — Boot Process

**Status:** Implemented and structurally verified. The kernel produces a valid
Multiboot 1 image; it has not been booted under QEMU in this environment (no
emulator installed here). The boot *path* below is real code, not a description
of a hypothetical one.

## Multiboot 1 header

THUOS boots via the Multiboot 1 specification, so any Multiboot-capable loader
(GRUB, QEMU's `-kernel`) can start it. The header is declared in
`kernel/boot/boot.S`:

```asm
.set MB_ALIGN,    1 << 0          /* align loaded modules on page boundaries */
.set MB_MEMINFO,  1 << 1          /* ask the loader for a memory map         */
.set MB_FLAGS,    MB_ALIGN | MB_MEMINFO          /* = 0x00000003             */
.set MB_MAGIC,    0x1BADB002
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)

.section .multiboot
.align 4
    .long MB_MAGIC
    .long MB_FLAGS
    .long MB_CHECKSUM
```

The three longs satisfy the Multiboot rule `magic + flags + checksum ≡ 0 (mod 2³²)`.
The linker script places `.multiboot` at the very start of `.text`, and the
build disables the build-id note, so the header lands within the first 8 KiB of
the file (verified at **file offset 4096**).

## `_start`

```asm
_start:
    movl  $stack_top, %esp     /* 16 KiB stack from .bss */
    pushl %ebx                 /* arg 2: Multiboot info pointer */
    pushl %eax                 /* arg 1: Multiboot magic (0x2BADB002 from GRUB) */
    call  kernel_main
    cli
.hang:
    hlt
    jmp .hang
```

The loader leaves the Multiboot magic in `eax` and the info-structure pointer in
`ebx`. `_start` pushes both as C arguments and calls `kernel_main`. If
`kernel_main` ever returns, the CPU is halted in a tight `hlt` loop.

## `kernel_main` bring-up

`kernel/core/kernel.c` runs the bring-up in a fixed, logged order. Each step
prints an `[ OK ]` line to both the VGA console and the serial port:

1. `serial_init()` — COM1 debug channel (loopback self-test).
2. `vga_init()` — clear screen, set the default color.
3. `gdt_init()` — flat 32-bit segments.
4. `idt_init()` — install an empty IDT.
5. `isr_init()` — exception gates 0–31.
6. `irq_init()` — remap the PIC and install IRQ gates 32–47.
7. `pit_init(100)` — 100 Hz timer on IRQ0.
8. `keyboard_init()` — IRQ1 handler + ring buffer.
9. Parse the Multiboot info: if `magic == 0x2BADB002` and the memory flag is set,
   record `mem_lower` / `mem_upper` for the shell's `mem`/`sysinfo` commands.
10. `sti` — enable interrupts (timer and keyboard go live).
11. `shell_run()` — enter the `thuos>` shell loop, which never returns.

## Expected console output

When booted on a Multiboot loader, the early console looks like:

```
THUOS 0.2.0 "Boot Seed"
THU Kernel - x86 (i386, 32-bit)
From-scratch OS foundation. Booting...

  [ OK ] Serial COM1 debug channel
  [ OK ] Global Descriptor Table
  [ OK ] Interrupt Descriptor Table
  [ OK ] CPU exception handlers (0-31)
  [ OK ] PIC remap + IRQ routing
  [ OK ] PIT timer @ 100 Hz
  [ OK ] PS/2 keyboard (IRQ1)
  [info] Multiboot memory: <low> KiB low, <high> KiB high

Welcome to THUOS.
Type 'help' to begin.

thuos>
```

> This is the *intended* output produced by the code above. It is reproduced in
> the THU Desktop preview's terminal and in `docs/design/`. It has not been
> captured from a live QEMU session here because QEMU is not installed in this
> build environment — see [`07_LIMITATIONS_AND_NEXT_STEPS.md`](07_LIMITATIONS_AND_NEXT_STEPS.md).

## Verifying the boot image

```bash
make verify
```

This builds the kernel and then checks (see `scripts/run_verify.sh` and
`scripts/verify_multiboot.py`):

- the ELF is 32-bit Intel 80386, statically linked;
- a Multiboot header exists within the first 8 KiB with a valid checksum;
- `_start`, `kernel_main`, `isr_handler`, `irq_handler`, `shell_run` are present.

To actually boot it on a machine that has QEMU:

```bash
qemu-system-i386 -kernel build/kernel.elf
# or, with the kernel log on your terminal:
qemu-system-i386 -kernel build/kernel.elf -serial stdio -display none
```
