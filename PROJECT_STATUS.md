# THUOS — Project Status

**Milestone:** 0.2 "Boot Seed" — Kernel Stability, Diagnostics, Timer, Preview
**Date of this status:** 2026-06-05
**Honesty rule:** every "Implemented" item is backed by a source file and passes
the build + structural verification in [`BUILD_VERIFICATION.txt`](BUILD_VERIFICATION.txt).
Nothing here claims a QEMU boot, ISO, or install that was not actually performed.

## Status legend

- **Implemented** — code exists, compiles, links, and is wired into the kernel; structurally verified.
- **Partially implemented** — some of the subsystem exists; the rest is designed/planned.
- **Designed** — specified in docs, not yet coded.
- **Planned** — on the roadmap, not yet specified in detail.

## Subsystem board

| Subsystem | Status | Evidence |
|-----------|--------|----------|
| Multiboot 1 boot + linker (load at 1 MiB) | Implemented | `kernel/boot/boot.S`, `linker.ld`; magic verified at file offset 4096 |
| VGA text console | Implemented | `kernel/drivers/vga.c` |
| Serial COM1 (loopback self-test) | Implemented | `kernel/drivers/serial.c` |
| `kprintf` formatted output | Implemented | `kernel/core/kprintf.c` |
| GDT (flat 32-bit) | Implemented | `kernel/arch/x86/gdt.c`, `gdt_flush.S` |
| IDT | Implemented | `kernel/arch/x86/idt.c` |
| CPU exceptions 0–31 | Implemented | `kernel/arch/x86/isr.c`, `isr_stubs.S` |
| PIC remap (0x20/0x28) | Implemented | `kernel/arch/x86/pic.c` |
| IRQ routing + EOI | Implemented | `kernel/arch/x86/irq.c` |
| PIT timer @ 100 Hz + uptime | Implemented | `kernel/arch/x86/pit.c` |
| PS/2 keyboard (IRQ1) | Implemented | `kernel/drivers/keyboard.c` |
| Panic / assert | Implemented | `kernel/core/panic.c` |
| Shell (`thuos>`, 17 commands) | Implemented | `kernel/shell/shell.c` |
| Freestanding `mem*`/`str*` | Implemented | `kernel/lib/string.c` |
| Unified driver registration framework | Designed | `docs/design/DRIVER_MODEL.md` |
| Physical memory manager / paging / heap | Planned (0.3) | `docs/08_ROADMAP.md` |
| VFS + initrd | Planned (0.4) | `docs/design/FILESYSTEM.md` |
| Userspace, syscalls, ELF loader | Planned (0.5) | roadmap |
| Framebuffer graphics + mouse | Planned (0.6) | `docs/design/GUI_THU_DESKTOP.md` |
| In-kernel THU Desktop / window manager | Planned (0.7) | `docs/design/GUI_THU_DESKTOP.md` |
| THUFS filesystem | Designed | `docs/design/FILESYSTEM.md` |
| `thupkg` package manager | Designed | `docs/design/PACKAGE_MANAGER.md` |
| Security / sandboxing / signed packages | Designed | `docs/design/SECURITY_MODEL.md` |
| THU Desktop concept preview (web) | Implemented (preview only) | `preview/thuos_preview.html` |

## Verified in this environment

- `make clean && make kernel` — links `build/kernel.elf` with no warnings.
- `file build/kernel.elf` — *ELF 32-bit LSB executable, Intel 80386, statically linked*.
- Multiboot 1 header present within the first 8 KiB; `magic + flags + checksum ≡ 0 (mod 2³²)`.
- Required symbols present: `_start`, `kernel_main`, `isr_handler`, `irq_handler`, `shell_run`.
- THU Desktop preview served over local HTTP (200 OK) and its JavaScript passes `node --check`.

## NOT verified here (and why)

| Claim | Why not verified | How to verify elsewhere |
|-------|------------------|-------------------------|
| Kernel boots and runs | `qemu-system-i386` not installed in this environment | `sudo apt-get install qemu-system-x86` then `make run` |
| Bootable ISO builds | `grub-mkrescue` / `xorriso` not installed | `sudo apt-get install grub-pc-bin grub-common xorriso mtools` then `make iso` |
| Hosted `gcc -m32` programs | 32-bit libc/multilib absent (only matters for hosted, **not** for this freestanding kernel) | `sudo apt-get install gcc-multilib` |

The kernel is written to be Multiboot-bootable; it simply has not been booted on
this machine. That distinction is kept explicit everywhere in this repo.

## Next milestone

**THUOS 0.3 — Memory Foundation:** parse the Multiboot memory map, add a physical
page allocator, a kernel heap, identity paging, and `memmap`/`pages`/`heap` shell
commands. Details in [`docs/07_LIMITATIONS_AND_NEXT_STEPS.md`](docs/07_LIMITATIONS_AND_NEXT_STEPS.md)
and [`docs/08_ROADMAP.md`](docs/08_ROADMAP.md).
