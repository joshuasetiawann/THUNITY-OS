# 01 — Kernel Architecture

**Status:** Implemented and build-verified for THU Kernel 0.2.0 "Boot Seed".

THU Kernel is a freestanding 32-bit x86 kernel. It does not link against a host
libc, makes no Linux syscalls, and provides its own type definitions, string
helpers, and formatted output.

## Source layout

```
kernel/
  boot/
    boot.S            Multiboot header + _start, sets up stack, calls kernel_main
  arch/x86/
    io.h              port I/O (inb/outb/inw/outw, io_wait)
    gdt.c / gdt.h     flat 32-bit Global Descriptor Table
    gdt_flush.S       loads GDTR and reloads segment registers
    idt.c / idt.h     Interrupt Descriptor Table (256 gates, lidt)
    isr.c / isr.h     CPU exception handlers 0-31, registers_t frame
    isr_stubs.S       assembly entry stubs for ISRs and IRQs + common stubs
    pic.c / pic.h     8259 PIC remap + EOI
    irq.c / irq.h     hardware IRQ routing (vectors 32-47)
    pit.c / pit.h     Programmable Interval Timer (IRQ0), tick/uptime
  core/
    kernel.c          kernel_main: bring-up sequence and hand-off to the shell
    kprintf.c / .h    minimal freestanding printf (VGA + serial)
    panic.c / .h      panic() + PANIC/ASSERT macros
  drivers/
    vga.c / vga.h     VGA 80x25 text console
    serial.c / .h     COM1 serial driver
    keyboard.c / .h   PS/2 keyboard (IRQ1)
  lib/
    types.h           freestanding fixed-width types (uint8_t..uint64_t, size_t)
    string.c / .h     memset/memcpy/memmove/memcmp + str* helpers
  shell/
    shell.c / shell.h interactive thuos> shell
  include/thuos/
    version.h         single source of truth for names and version strings
```

## Build model

The kernel is compiled with GCC in freestanding mode and linked with a custom
script. Key flags (see `Makefile`):

```
CFLAGS  = -m32 -std=gnu11 -ffreestanding -O2
          -fno-stack-protector -fno-pic -fno-pie
          -fno-builtin -fno-tree-loop-distribute-patterns
          -Wall -Wextra
LDFLAGS = -m32 -ffreestanding -nostdlib -no-pie -static
          -Wl,--build-id=none -T linker.ld
```

Notes:
- `-fno-builtin` and `-fno-tree-loop-distribute-patterns` stop GCC from rewriting
  our hand-written `memset`/`memcpy` into self-referential library calls.
- `--build-id=none` keeps a `.note` section from being emitted before `.text`,
  which would otherwise push the Multiboot header past the 8 KiB limit.
- `-no-pie -static` produce a plain static ELF with no interpreter — required for
  a kernel image.

The freestanding kernel builds even though *hosted* `gcc -m32` does not work in
this environment (the 32-bit libc/multilib is absent). The kernel needs neither.

## Memory layout

The linker (`linker.ld`) loads the kernel at the conventional **1 MiB** mark and
places the Multiboot header first so the loader can find it:

```
. = 1M
.text   (.multiboot first, then code)
.rodata
.data
.bss    (16 KiB stack lives here, plus IDT/GDT tables)
thuos_kernel_start / thuos_kernel_end symbols bracket the image
```

## Boot-to-shell flow

```
_start (boot.S)
  └─ set up stack, push Multiboot magic + info
     └─ kernel_main (core/kernel.c)
         ├─ serial_init()         COM1 debug channel
         ├─ vga_init()            clear screen, set color
         ├─ gdt_init()            flat segments
         ├─ idt_init()            empty IDT
         ├─ isr_init()            install exception gates 0-31
         ├─ irq_init()            PIC remap + IRQ gates 32-47
         ├─ pit_init(100)         100 Hz timer on IRQ0
         ├─ keyboard_init()       IRQ1 handler + ring buffer
         ├─ parse Multiboot mem   record mem_lower/mem_upper if present
         ├─ sti                   enable interrupts
         └─ shell_run()           thuos> prompt loop (never returns)
```

## Design principles

- **No silent failure.** Faults route through the panic system or a named CPU
  exception report.
- **Separation of concerns.** Arch code, drivers, core, lib, and shell are
  distinct directories with clear headers.
- **Freestanding.** No assumptions about a host runtime.
- **Honest growth.** New subsystems land as their own `.c`/`.h` pairs and are
  documented as they are built.

See [`02_BOOT_PROCESS.md`](02_BOOT_PROCESS.md) and
[`03_INTERRUPT_SYSTEM.md`](03_INTERRUPT_SYSTEM.md) for subsystem detail.
