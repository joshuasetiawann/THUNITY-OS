# THUOS

**A from-scratch personal operating system** — a real, bootable x86 kernel today,
built incrementally toward a calm, local-first desktop OS tomorrow.

> **THUOS** is not a Linux distribution, not an Electron app, and not a website.
> It is a freestanding operating-system kernel written in C and assembly, plus a
> separate **THU Desktop** concept preview that shows the intended visual
> direction while the kernel grows up to it.

- **Project:** THUOS · **Kernel:** THU Kernel · **Desktop:** THU Desktop
- **Filesystem (planned):** THUFS · **Package manager (planned):** thupkg
- **Version:** `0.2.0` "Boot Seed" · **Arch:** x86 (i386, 32-bit) · **Boot:** Multiboot 1 (GRUB-compatible)

---

## What works today (build-verified)

THU Kernel 0.2.0 compiles and links into a **valid 32-bit Multiboot ELF**. The
following subsystems are implemented and wired into `kernel_main()`:

| Area | Status | Source |
|------|--------|--------|
| Multiboot 1 boot stub + 1 MiB load | ✅ Implemented | `kernel/boot/boot.S`, `linker.ld` |
| VGA 80×25 text console (scroll, cursor, color) | ✅ Implemented | `kernel/drivers/vga.c` |
| COM1 serial debug channel (loopback self-test) | ✅ Implemented | `kernel/drivers/serial.c` |
| `kprintf` (VGA + serial mirror) | ✅ Implemented | `kernel/core/kprintf.c` |
| GDT (flat 32-bit, 5 descriptors) | ✅ Implemented | `kernel/arch/x86/gdt.c` |
| IDT + CPU exceptions 0–31 (named crash report) | ✅ Implemented | `kernel/arch/x86/idt.c`, `isr.c`, `isr_stubs.S` |
| PIC remap + IRQ routing + EOI | ✅ Implemented | `kernel/arch/x86/pic.c`, `irq.c` |
| PIT timer @ 100 Hz + uptime | ✅ Implemented | `kernel/arch/x86/pit.c` |
| PS/2 keyboard (IRQ1, scancode set 1, shift) | ✅ Implemented | `kernel/drivers/keyboard.c` |
| Panic / assert system | ✅ Implemented | `kernel/core/panic.c` |
| Interactive shell (`thuos>`, 17 commands) | ✅ Implemented | `kernel/shell/shell.c` |
| Freestanding `mem*`/`str*` lib | ✅ Implemented | `kernel/lib/string.c` |

> **What "Implemented" means here:** the code compiles, links, and is integrated,
> and the kernel passes structural verification (valid Multiboot header, correct
> ELF class, required symbols present). It has **not** been booted under QEMU *in
> this build environment* because QEMU is not installed here. See
> [`docs/07_LIMITATIONS_AND_NEXT_STEPS.md`](docs/07_LIMITATIONS_AND_NEXT_STEPS.md)
> and [`BUILD_VERIFICATION.txt`](BUILD_VERIFICATION.txt) for the honest details.

## Planned (not done yet)

Memory manager (0.3) · VFS + initrd (0.4) · userspace + syscalls (0.5) ·
framebuffer graphics (0.6) · in-kernel THU Desktop (0.7) · real `thupkg`
backend · installer ISO. See [`docs/08_ROADMAP.md`](docs/08_ROADMAP.md).

---

## Quick start

```bash
make kernel     # build build/kernel.elf (freestanding 32-bit)
make verify     # check ELF class, Multiboot header+checksum, symbols -> BUILD_VERIFICATION.txt
make status     # one-screen honest project status
make demo       # serve the THU Desktop preview at http://localhost:8080
make clean      # remove build artifacts
```

Targets that need tools not present in every environment fail **honestly**:

```bash
make iso        # builds a bootable ISO IF grub-mkrescue + xorriso exist, else prints how to install them
make run        # boots in QEMU IF qemu-system-i386 exists, else prints how to install it
make run-serial # same as run, with serial output on stdio
```

## THU Desktop preview

Open the self-contained, no-CDN preview in any browser:

```
preview/thuos_preview.html
```

It boots into a concept desktop with a **login screen**, a **dock/sidebar**,
**draggable windows**, a **terminal** that simulates the `thuos>` shell, a
**Kernel Status** panel, and a **Build Verification** window. It is clearly
labeled a *concept preview* — it is **not** the real kernel GUI. See
[`docs/06_THU_DESKTOP_PREVIEW.md`](docs/06_THU_DESKTOP_PREVIEW.md).

## Repository layout

```
kernel/        THU Kernel source (boot, arch/x86, core, drivers, lib, shell)
linker.ld      Loads the kernel at 1 MiB, Multiboot header first
Makefile       Build / verify / demo / iso / run targets
grub.cfg       GRUB menu used when building an ISO
scripts/       verify_multiboot.py, run_verify.sh, status.sh
preview/       THU Desktop concept preview (self-contained HTML)
docs/          Milestone documentation (00–08) + docs/design/ deep specs
```

## Documentation

| Doc | Topic |
|-----|-------|
| [00_OVERVIEW](docs/00_OVERVIEW.md) | Vision, philosophy, architecture layers |
| [01_KERNEL_ARCHITECTURE](docs/01_KERNEL_ARCHITECTURE.md) | Code structure, build model, memory layout |
| [02_BOOT_PROCESS](docs/02_BOOT_PROCESS.md) | Multiboot, `_start`, `kernel_main` bring-up |
| [03_INTERRUPT_SYSTEM](docs/03_INTERRUPT_SYSTEM.md) | GDT, IDT, exceptions, PIC, IRQs, PIT |
| [04_DRIVER_MODEL](docs/04_DRIVER_MODEL.md) | Implemented drivers + future driver framework |
| [05_SHELL_COMMANDS](docs/05_SHELL_COMMANDS.md) | Full `thuos>` command reference |
| [06_THU_DESKTOP_PREVIEW](docs/06_THU_DESKTOP_PREVIEW.md) | The web concept preview, honestly framed |
| [07_LIMITATIONS_AND_NEXT_STEPS](docs/07_LIMITATIONS_AND_NEXT_STEPS.md) | What is unverified and why |
| [08_ROADMAP](docs/08_ROADMAP.md) | Release roadmap 0.2 → 1.0 |
| [PROJECT_STATUS](PROJECT_STATUS.md) | Subsystem status board |

## Principles

Local-first · privacy-first · honest engineering · emulator-first development ·
no destructive disk writes · no faked results. THUOS keeps the boot path working
after every milestone and never claims a build, boot, or install that did not
actually happen.
