# 03 — Interrupt System

**Status:** Implemented and build-verified. GDT, IDT, CPU exceptions 0–31, PIC
remap, IRQ routing, the PIT timer (IRQ0), and the keyboard (IRQ1) are all coded
and wired into `kernel_main`.

## Global Descriptor Table (GDT)

`kernel/arch/x86/gdt.c` installs a flat 32-bit model with five descriptors:

| Index | Selector | Purpose | Access |
|-------|----------|---------|--------|
| 0 | 0x00 | null | 0x00 |
| 1 | 0x08 | kernel code (ring 0) | 0x9A |
| 2 | 0x10 | kernel data (ring 0) | 0x92 |
| 3 | 0x18 | user code (ring 3) | 0xFA |
| 4 | 0x20 | user data (ring 3) | 0xF2 |

All segments span the full 4 GiB (`limit = 0xFFFFFFFF`, granularity `0xCF`).
`gdt_flush.S` loads the GDTR and reloads `ds/es/fs/gs/ss` plus a far jump to
reload `cs` to `0x08`. The user-mode descriptors exist for the future userspace
milestone; the kernel currently runs entirely in ring 0.

## Interrupt Descriptor Table (IDT)

`kernel/arch/x86/idt.c` defines 256 gates and loads the IDTR with `lidt`.
`idt_set_gate(num, handler, selector, flags)` fills a gate; ISR and IRQ
installers call it with selector `0x08` and flags `0x8E` (present, ring 0,
32-bit interrupt gate).

## CPU exceptions (vectors 0–31)

`kernel/arch/x86/isr_stubs.S` provides one assembly stub per exception. Stubs for
exceptions that the CPU pushes an error code for (8, 10–14) keep that code;
the rest push a dummy `0` so every path produces the same stack shape. Each stub
pushes the vector number and jumps to a common stub that builds a `registers_t`
frame and calls the C dispatcher `isr_handler`.

`registers_t` (in `isr.h`) mirrors the stack exactly:

```c
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha   */
    uint32_t int_no, err_code;                        /* stub    */
    uint32_t eip, cs, eflags, useresp, ss;            /* CPU     */
} registers_t;
```

If no handler is registered for the vector, `isr_handler` prints a controlled
crash report (vector number, human-readable name, error code, `eip`, `cs`,
`eflags`, and the general registers) on a red screen and halts. The 32 exception
names (e.g. *Divide-by-zero*, *Invalid opcode*, *General protection fault*,
*Page fault*) live in `isr.c`.

You can exercise this path safely from the shell with `crash div0`, which
performs a divide-by-zero and lands in the exception 0 report.

## PIC remap and IRQs (vectors 32–47)

The two 8259 PICs are remapped in `kernel/arch/x86/pic.c` so hardware IRQs do not
overlap CPU exception vectors:

- Master PIC → vectors 0x20–0x27 (IRQ 0–7)
- Slave PIC → vectors 0x28–0x2F (IRQ 8–15)

`kernel/arch/x86/irq.c` installs the 16 IRQ gates and routes each interrupt to a
registered handler via `irq_register_handler(irq, fn)`. The common IRQ stub
shares the same `registers_t` mechanism as exceptions. After every IRQ,
`irq_handler` sends an end-of-interrupt (`pic_send_eoi`) to the correct PIC.

## Timer — PIT on IRQ0

`kernel/arch/x86/pit.c` programs PIT channel 0 to 100 Hz (mode 3, square wave)
and increments a tick counter on each IRQ0. It exposes:

- `pit_ticks()` — raw tick count;
- `pit_seconds()` — `ticks / frequency`;
- `pit_frequency()` — configured Hz.

These back the shell's `uptime`, `ticks`, and `sysinfo` commands.

## Keyboard — PS/2 on IRQ1

`kernel/drivers/keyboard.c` reads scancodes from port `0x60` on IRQ1, translates
scancode set 1 to ASCII (with a separate shifted table), tracks the shift state,
and pushes characters into a 128-byte ring buffer. `keyboard_getchar()` blocks
with `sti; hlt` until a character is available, so the shell sleeps the CPU while
waiting for input instead of busy-spinning.

## Interrupt-safety notes

- Stubs `cli` on entry and the common stub `sti` + `iret` on exit.
- Handlers are short and do not re-enable nested IRQs.
- The keyboard handler drops input on buffer overflow rather than corrupting state.
