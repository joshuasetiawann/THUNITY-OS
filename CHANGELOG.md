# Changelog

All notable changes to THUOS are documented here. Format loosely follows
[Keep a Changelog](https://keepachangelog.com/). Versions track the THU Kernel.

## [0.2.0] — "Boot Seed" — 2026-06-05

Milestone 0.2: Kernel Stability, Diagnostics, Timer, and Preview. This is the
first substantial THUOS milestone: a real freestanding x86 kernel that builds
into a valid Multiboot ELF, plus an interactive THU Desktop concept preview.

### Added — kernel
- Multiboot 1 boot stub and 1 MiB load via `kernel/boot/boot.S` + `linker.ld`.
- VGA 80×25 text console with scrolling, hardware cursor, and color (`drivers/vga.c`).
- COM1 serial debug channel with a loopback self-test (`drivers/serial.c`).
- `kprintf` formatted output mirrored to VGA and serial (`core/kprintf.c`).
- Flat 32-bit GDT with 5 descriptors and segment reload (`arch/x86/gdt.c`, `gdt_flush.S`).
- IDT plus CPU exception handlers for vectors 0–31 with named crash reports
  (`arch/x86/idt.c`, `isr.c`, `isr_stubs.S`).
- 8259 PIC remap to 0x20/0x28 and IRQ routing with end-of-interrupt
  (`arch/x86/pic.c`, `irq.c`).
- PIT timer at 100 Hz with a tick counter and `uptime` (`arch/x86/pit.c`).
- PS/2 keyboard driver on IRQ1, scancode set 1, shift handling, ring buffer
  (`drivers/keyboard.c`).
- Kernel panic / assert system that halts loudly instead of failing silently
  (`core/panic.c`).
- Interactive `thuos>` shell with 17 commands: `help`, `about`, `version`,
  `status`, `sysinfo`, `uptime`, `ticks`, `mem`, `echo`, `banner`, `color`,
  `thupkg`, `clear`, `crash`, `reboot`, `halt` (`shell/shell.c`).
- Freestanding `mem*` / `str*` helpers; no host libc dependency (`lib/string.c`).

### Added — build & verification
- `Makefile` targets: `kernel`, `verify`, `status`, `demo`, `iso`, `run`,
  `run-serial`, `clean`. `iso` and `run` degrade gracefully when their tools
  are missing instead of pretending to succeed.
- `scripts/verify_multiboot.py` — confirms the Multiboot magic and checksum sit
  within the first 8 KiB of the kernel image.
- `scripts/run_verify.sh` — full verification battery written to `BUILD_VERIFICATION.txt`.
- `scripts/status.sh` — one-screen honest project status.

### Added — THU Desktop preview
- `preview/thuos_preview.html` — self-contained (no CDN) concept preview with a
  boot screen, **login screen**, desktop, dock/sidebar, draggable windows, a
  terminal that simulates the `thuos>` shell (`help`, `about`, `clear`, `uptime`,
  `mem`, `version`, `echo`, `panic`, `reboot`, `halt`, and more), a **Kernel
  Status** panel, and a **Build Verification** window.
- `preview/thuos_desktop_preview.html` — forwards to the interactive preview.

### Added — documentation
- `README.md`, `PROJECT_STATUS.md`, `CHANGELOG.md`, `BUILD_VERIFICATION.txt`.
- `docs/00`–`docs/08` milestone docs and `docs/design/` deep specifications.

### Known limitations (honest)
- The kernel has **not** been booted in this environment: `qemu-system-i386` is
  not installed here. Boot is therefore unverified locally; see `make run`.
- No bootable ISO was produced here: `grub-mkrescue` / `xorriso` are not installed.
- No memory manager, filesystem, userspace, networking, or graphics yet — those
  are designed/planned for later milestones.

## [0.0.1] — "Boot Seed (baseline)"
- Project conception: a from-scratch, Multiboot, x86 32-bit kernel with VGA text
  output, keyboard input, and a minimal shell, established as the THUOS direction.
