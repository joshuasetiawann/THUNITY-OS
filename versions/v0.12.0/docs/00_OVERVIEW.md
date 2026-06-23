# 00 — THUOS Overview

**Status:** THU Kernel 0.2.0 "Boot Seed" is implemented and build-verified.
Everything beyond the kernel foundation is designed or planned.

## Vision

THUOS is a personal operating system built in stages, from a small bootable
kernel toward a complete, calm, local-first desktop OS. THUOS is not a Linux
theme, not a web dashboard, and not an empty simulation. The goal is a system
with its own kernel, boot process, driver model, memory manager, filesystem,
shell, GUI, built-in apps, package manager, installer, and honest engineering
documentation.

Design direction: **local-first, privacy-first, clean, fast, auditable, and with
its own identity.** THUOS learns from Linux (modular, transparent), Windows
(familiar UX, device management), and macOS (visual consistency, polish), but it
is not a raw clone of any of them.

## Naming (do not change)

| Thing | Name |
|-------|------|
| Operating system | **THUOS** |
| Kernel | **THU Kernel** |
| Desktop | **THU Desktop** |
| Filesystem (planned) | **THUFS** |
| Package manager (planned) | **thupkg** |
| App bundle (planned) | **.thuapp** |

## Architecture layers

```
Hardware / Emulator
  -> Bootloader (Multiboot 1 / GRUB)
  -> THU Kernel entry (boot.S, kernel_main)
  -> Arch core: GDT, IDT, ISR, PIC, IRQ, PIT
  -> Drivers: VGA, serial, keyboard  (framebuffer, disk, net -> later)
  -> Memory: PMM, paging, heap        (Milestone 0.3)
  -> Filesystem: VFS, initrd, THUFS    (Milestone 0.4)
  -> Userspace: syscalls, ELF, init    (Milestone 0.5)
  -> THU Shell + core utilities
  -> THU Desktop + apps                (Milestone 0.6+)
  -> thupkg package ecosystem          (later)
```

The **bold/implemented** part of this stack today is everything from the
bootloader through the shell. Memory and everything below it are forward-looking.

## What exists right now

- A freestanding 32-bit kernel that compiles and links into a valid Multiboot ELF.
- Text-mode console + serial logging, full interrupt plumbing (GDT/IDT/PIC/IRQ),
  a 100 Hz timer, a keyboard driver, a panic system, and an interactive shell.
- A self-contained **THU Desktop concept preview** (web) for visual direction.
- Verification tooling and honest status documentation.

## What does NOT exist yet

Memory management, filesystem, userspace/processes, networking, sound, USB,
graphics/framebuffer, a real package manager, and an installer. These are tracked
honestly in [`08_ROADMAP.md`](08_ROADMAP.md) and [`../PROJECT_STATUS.md`](../PROJECT_STATUS.md).

## Document index

| Doc | Topic |
|-----|-------|
| [01_KERNEL_ARCHITECTURE](01_KERNEL_ARCHITECTURE.md) | Code structure, build model, memory layout |
| [02_BOOT_PROCESS](02_BOOT_PROCESS.md) | Multiboot header, `_start`, bring-up sequence |
| [03_INTERRUPT_SYSTEM](03_INTERRUPT_SYSTEM.md) | GDT, IDT, exceptions, PIC, IRQs, timer, keyboard |
| [04_DRIVER_MODEL](04_DRIVER_MODEL.md) | Implemented drivers and the future framework |
| [05_SHELL_COMMANDS](05_SHELL_COMMANDS.md) | Full `thuos>` command reference |
| [06_THU_DESKTOP_PREVIEW](06_THU_DESKTOP_PREVIEW.md) | The web concept preview |
| [07_LIMITATIONS_AND_NEXT_STEPS](07_LIMITATIONS_AND_NEXT_STEPS.md) | Honest limits + next steps |
| [08_ROADMAP](08_ROADMAP.md) | Release roadmap 0.2 → 1.0 |
| [design/](design/) | Deeper forward-looking specs (filesystem, drivers, security, GUI, packages) |

## Non-negotiables

- The name is **THUOS**. Do not rename it.
- Never fake status. Separate *implemented*, *in progress*, and *planned*.
- Emulator-first development; never write to real disks without explicit consent.
- Keep the kernel buildable after every milestone.
