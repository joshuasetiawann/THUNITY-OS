<div align="center">

# THUOS

**A from-scratch personal operating system** — a real, bootable x86 kernel with a
modern desktop, built incrementally toward a calm, local-first OS.

[![version](https://img.shields.io/badge/version-0.19.0%20%22USB%22-6aa9ff?style=flat-square)](CHANGELOG.md)
[![arch](https://img.shields.io/badge/arch-x86%20·%20i386%2032--bit-3d8bdc?style=flat-square)](#)
[![boot](https://img.shields.io/badge/boot-QEMU%20%2B%20GRUB%20ISO%20✓-2bb5a0?style=flat-square)](docs/THUOS_REALITY_CHECK.md)
[![input](https://img.shields.io/badge/input-PS%2F2%20%2B%20USB--HID%20(xHCI)-ff9f1c?style=flat-square)](#)
[![tests](https://img.shields.io/badge/tests-host%20units%20%2B%20boot%20smoke-7c8aa0?style=flat-square)](#quick-start)
[![made with](https://img.shields.io/badge/made%20with-C%20%2B%20x86%20asm-555?style=flat-square)](#)

<img src="preview/screenshots/desktop.png" width="780" alt="The THU Desktop running in QEMU"><br>
<sub><b>Real screenshot</b> — THUOS 0.19.0 booted in QEMU (not a mockup). Top bar + clock, a rounded terminal window, and a dock of apps.</sub>

</div>

> **THUOS** is a freestanding operating-system kernel written in C and x86 assembly.
> It boots via Multiboot, brings up paging, cooperative multitasking, a RAM
> filesystem, ring-3 user mode, a **1024×768 truecolor desktop**, and **USB-HID
> keyboard + mouse** — all **boot-verified in QEMU** on every push. It is *not* a
> Linux distribution, an Electron app, or a website.

- **Project:** THUOS · **Kernel:** THU Kernel · **Desktop:** THU Desktop
- **Filesystem (planned):** THUFS · **Package manager (planned):** thupkg
- **Version:** `0.20.0` "AI-Native" · **Arch:** x86 (i386, 32-bit) · **Boot:** Multiboot 1 · **Input:** PS/2 **+ USB-HID (xHCI)** · **Boot status:** ✅ verified in QEMU (`-kernel`) **and via a GRUB ISO**
- **AI-native foundation (host-tested):** in-kernel AI service/model/task/permission/audit core + `ai` shell namespace — **design-only bridge**, *no* inference/networking/Docker/Python yet ([doc](docs/39_AI_NATIVE_GENERAL_OS_FOUNDATION.md) · [bridge strategy](docs/40_THUNITY_AI_BRIDGE_STRATEGY.md))

---

## Screenshots

> All real, captured from the running kernel in QEMU — **not** the HTML concept preview.

<table>
<tr>
<td align="center" width="50%"><img src="preview/screenshots/terminal.png" width="400" alt="Terminal"><br><b>Terminal</b> — the <code>thuos&gt;</code> shell (29 commands)</td>
<td align="center" width="50%"><img src="preview/screenshots/settings.png" width="400" alt="Settings"><br><b>Settings</b> — live themes, Display, Devices, About</td>
</tr>
<tr>
<td align="center"><img src="preview/screenshots/files.png" width="400" alt="Files"><br><b>Files</b> — browse the in-RAM filesystem</td>
<td align="center"><img src="preview/screenshots/calculator.png" width="400" alt="Calculator"><br><b>Calculator</b> — clickable keypad</td>
</tr>
<tr>
<td align="center"><img src="preview/screenshots/notes.png" width="400" alt="Notes"><br><b>Notes</b> — typed via the <b>USB keyboard</b>, autosaved to ramfs</td>
<td align="center"><img src="preview/screenshots/paint.png" width="400" alt="Paint"><br><b>Paint</b> — draw with the <b>USB mouse</b></td>
</tr>
</table>

---

## What works today (boot-verified in QEMU)

THU Kernel **boots in QEMU and reaches its `thuos>` shell** — verified
automatically on every push by the CI `boot-smoke` job (`scripts/boottest.sh`
captures COM1 serial and checks the boot markers). The following subsystems are
implemented and wired into `kernel_main()`:

| Area | Status | Source |
|------|--------|--------|
| Multiboot 1 boot stub + 1 MiB load | ✅ Implemented | `kernel/boot/boot.S`, `linker.ld` |
| VGA 80×25 text console (scroll, cursor, color) | ✅ Implemented | `kernel/drivers/vga.c` |
| COM1 serial debug channel (loopback self-test) | ✅ Implemented | `kernel/drivers/serial.c` |
| `kprintf` (VGA + serial mirror) | ✅ Implemented | `kernel/core/kprintf.c` |
| GDT (flat 32-bit, 6 descriptors incl. TSS) | ✅ Implemented | `kernel/arch/x86/gdt.c`, `tss.c` |
| IDT + CPU exceptions 0–31 (named crash report) | ✅ Implemented | `kernel/arch/x86/idt.c`, `isr.c`, `isr_stubs.S` |
| PIC remap + IRQ routing + EOI | ✅ Implemented | `kernel/arch/x86/pic.c`, `irq.c` |
| PIT timer @ 100 Hz + uptime | ✅ Implemented | `kernel/arch/x86/pit.c` |
| PS/2 keyboard (IRQ1, scancode set 1, shift) | ✅ Implemented | `kernel/drivers/keyboard.c` |
| **USB: xHCI controller + USB-HID boot keyboard & mouse (real-laptop input, no PS/2)** | ✅ Boot-verified (QEMU `qemu-xhci`) | `kernel/drivers/usb/xhci.c` |
| Panic / assert system | ✅ Implemented | `kernel/core/panic.c` |
| Interactive shell (`thuos>`, 29 commands) | ✅ Implemented | `kernel/shell/shell.c` |
| Freestanding `mem*`/`str*` lib | ✅ Implemented | `kernel/lib/string.c` |
| Multiboot memory-map parsing | ✅ Implemented | `kernel/mm/multiboot.h`, `pmm.c` |
| Physical memory manager (4 KiB frames) + protected-region reservation | ✅ Implemented | `kernel/mm/pmm.c`, `frame_bitmap.c` (unit-tested: `tests/test_pmm.c`) |
| Memory shell commands (`memmap`, `pages`, `allocpage`, `freepage`) | ✅ Implemented | `kernel/shell/shell.c` |
| Kernel heap (`kmalloc`/`kfree`) | ✅ Implemented + host-tested | `kernel/mm/kheap_core.c`, `kheap.c` (`tests/test_kheap.c`) |
| Paging tables + translation | ✅ Host-tested + boot-verified (CR0.PG on) | `kernel/mm/vmm_core.c`, `vmm.c` (`tests/test_vmm.c`) |
| Round-robin scheduler policy core | ✅ Host-tested | `kernel/sched/sched_core.c`, `sched.c` (`tests/test_sched.c`) |
| Cooperative multitasking + RAM filesystem + `int 0x80` syscalls | ✅ Boot-verified (CI) | `kernel/sched/coop.c`, `kernel/fs/fs.c`, `kernel/arch/x86/syscall.c` |
| User mode (ring 3): TSS + `iret` to CPL 3 + `int 0x80` from userspace | ✅ Host-tested + boot-verified (CI) | `kernel/arch/x86/usermode.c`, `usermode_entry.S`, `tss.c` (`tests/test_usermode.c`) |
| **Modern desktop: 1024×768×32 framebuffer (bootloader FB on real HW, Bochs VBE fallback) + graphical terminal** | ✅ Boot-verified (CI `-kernel` + GRUB ISO) + screenshot | `kernel/drivers/lfb.c`, `kernel/gui/gconsole.c`, `desktop.c` |
| **Mouse (PS/2 + USB) + clickable dock + apps (Calculator, Files, System, About)** | ✅ Boot-verified (CI) + screenshot | `kernel/drivers/mouse.c`, `kernel/gui/apps.c`, `desktop.c` |
| **Settings menu (live themes) + Notes (ramfs) + Paint (mouse drawing)** | ✅ Boot-verified (CI) + screenshot | `kernel/gui/apps.c`, `desktop.c` |
| **AI-native core: service/model/task/permission/audit (local-only policy)** | ✅ Host-tested (`tests/test_ai.c`) + `ai` namespace boot-verified | `kernel/ai/ai_core.c`, `ai.c` |
| **OS feature/status registry + `features`/`dmesg`/`history`** | ✅ Host-tested (`tests/test_features.c`) + boot-verified | `kernel/os/feature_registry.c`, `osreport.c` |
| Thunity AI bridge (talk to a local Linux AI server) | ⚠️ **Design-only** — no networking/inference in THUOS yet | [doc 40](docs/40_THUNITY_AI_BRIDGE_STRATEGY.md) |
| Boot in QEMU → `thuos>` (serial smoke-test) | ✅ Boot-verified (CI) | `scripts/boottest.sh`, CI `boot-smoke` |

> **Honesty:** logic is **host-tested** (`make test`: PMM, heap, paging, scheduler)
> and the kernel is **boot-verified in QEMU** by CI (`make boottest` → reaches
> `thuos>` over serial; the screenshots above are from QEMU). It has **not** been
> run on physical hardware yet. See
> [`docs/THUOS_REALITY_CHECK.md`](docs/THUOS_REALITY_CHECK.md) for the honest big picture.

## Direction & what's next

THUOS's honest strategy — it cannot out-engineer Windows/macOS head-on (see
[`docs/THUOS_REALITY_CHECK.md`](docs/THUOS_REALITY_CHECK.md)) — is a focused wedge:
a **capability-secured, local-first, fast-boot developer/AI-agent OS that runs in
a VM/microVM** (sidestepping the driver moat). See
[`docs/THUOS_VISION.md`](docs/THUOS_VISION.md) and
[`docs/THUOS_ARCHITECTURE.md`](docs/THUOS_ARCHITECTURE.md).

**Done so far** (boot-verified in QEMU): paging · context switch · multitasking ·
RAM filesystem · `int 0x80` syscalls · ring 3 · a modern 1024×768 truecolor
desktop · a PS/2 **and USB** mouse + clickable dock · a Settings menu (live
themes) and apps: Terminal, Files, Notes, Calculator, Paint · **USB-HID keyboard
& mouse via xHCI** (input on real laptops with no PS/2).
**Next** (staged): a UEFI boot path (grub-efi) so the ISO boots a modern laptop ·
movable windows · an ELF loader + per-process isolation so apps load *from files*.
**Honest limits:** camera, Wi-Fi and Bluetooth are not supported (no device in
QEMU / vendor firmware+drivers out of scope) — Settings ▸ Devices says so plainly.

---

## Quick start

```bash
make kernel     # build build/kernel.elf (freestanding 32-bit)
make verify     # ELF class, Multiboot header+checksum, symbols, allocator test -> BUILD_VERIFICATION.txt
make test       # host unit tests: PMM + kernel heap + paging + scheduler (native gcc)
make boottest   # BOOT THUOS in QEMU and verify it reaches thuos> (skips if no QEMU)
make status     # one-screen honest project status
make demo       # serve the THU Desktop concept preview at http://localhost:8080
make clean      # remove build artifacts
```

Targets that need tools not present in every environment fail **honestly**:

```bash
make iso        # builds a bootable ISO IF grub-mkrescue + xorriso exist, else prints how to install them
make run        # boots in QEMU IF qemu-system-i386 exists, else prints how to install it
make run-serial # same as run, with serial output on stdio
```

To reproduce the screenshots above (USB input path), boot with a virtual xHCI
controller and USB HID devices:

```bash
qemu-system-i386 -kernel build/kernel.elf -vga std \
  -device qemu-xhci -device usb-kbd -device usb-mouse
```

## THU Desktop — real GUI vs. concept preview

The screenshots above are the **real kernel GUI**. There is *also* a separate,
self-contained **HTML concept preview** that explores the intended visual
direction (login screen, draggable windows, richer chrome) ahead of the kernel:

```
preview/thuos_preview.html      # open in any browser — clearly a concept, not the kernel
```

See [`docs/06_THU_DESKTOP_PREVIEW.md`](docs/06_THU_DESKTOP_PREVIEW.md).

## Repository layout

```
kernel/             THU Kernel source (boot, arch/x86, core, drivers, lib, shell, mm, sched, fs, gui)
kernel/drivers/usb/ xHCI host controller + USB-HID keyboard/mouse
linker.ld           Loads the kernel at 1 MiB, Multiboot header first
Makefile            Build / verify / test / boottest / demo / iso / run targets
grub.cfg            GRUB menu used when building an ISO
scripts/            verify_multiboot.py, boottest.sh, status.sh
preview/            THU Desktop concept preview (HTML) + screenshots/ (real QEMU captures)
docs/               Milestone documentation (00–11) + vision/architecture deep specs
tests/              Host unit tests (PMM, heap, paging, scheduler, fs, syscall, usermode)
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
| [REAL_HARDWARE](docs/REAL_HARDWARE.md) | What it takes to boot on a physical machine |
| [THUOS_VISION](docs/THUOS_VISION.md) | Project vision & the winnable wedge |
| [THUOS_ARCHITECTURE](docs/THUOS_ARCHITECTURE.md) | Studied Linux/XNU/NT/seL4 → THUOS design |
| [THUOS_REALITY_CHECK](docs/THUOS_REALITY_CHECK.md) | Deep research: can THUOS rival the giants? |
| [BOOT_THUOS](BOOT_THUOS.md) | How to boot THUOS yourself (QEMU/ISO) |

## Principles

Local-first · privacy-first · honest engineering · emulator-first development ·
no destructive disk writes · no faked results. THUOS keeps the boot path working
after every milestone and never claims a build, boot, or install that did not
actually happen — the screenshots above are real QEMU captures, and the version
archive on [`master`](../../tree/master) keeps every release's source, dated.
