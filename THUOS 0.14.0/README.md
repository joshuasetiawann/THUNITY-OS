# THUOS

**A from-scratch personal operating system** — a real, bootable x86 kernel today,
built incrementally toward a calm, local-first desktop OS tomorrow.

> **THUOS** is not a Linux distribution, not an Electron app, and not a website.
> It is a freestanding operating-system kernel written in C and assembly, plus a
> separate **THU Desktop** concept preview that shows the intended visual
> direction while the kernel grows up to it.

- **Project:** THUOS · **Kernel:** THU Kernel · **Desktop:** THU Desktop
- **Filesystem (planned):** THUFS · **Package manager (planned):** thupkg
- **Version:** `0.14.0` "Aurora" · **Arch:** x86 (i386, 32-bit) · **Boot:** Multiboot 1 · **Boot status:** ✅ boot-verified in QEMU (CI) · with a **modern 1024×768 truecolor desktop**

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
| Panic / assert system | ✅ Implemented | `kernel/core/panic.c` |
| Interactive shell (`thuos>`, 29 commands) | ✅ Implemented | `kernel/shell/shell.c` |
| Freestanding `mem*`/`str*` lib | ✅ Implemented | `kernel/lib/string.c` |
| Multiboot memory-map parsing | ✅ Implemented | `kernel/mm/multiboot.h`, `pmm.c` |
| Physical memory manager (4 KiB frames) + protected-region reservation | ✅ Implemented | `kernel/mm/pmm.c`, `frame_bitmap.c` (unit-tested: `tests/test_pmm.c`) |
| Memory shell commands (`memmap`, `pages`, `allocpage`, `freepage`) | ✅ Implemented | `kernel/shell/shell.c` |
| Kernel heap (`kmalloc`/`kfree`) | ✅ Implemented + host-tested | `kernel/mm/kheap_core.c`, `kheap.c` (`tests/test_kheap.c`) |
| Paging tables + translation (staged, not enabled) | ✅ Host-tested | `kernel/mm/vmm_core.c`, `vmm.c` (`tests/test_vmm.c`) |
| Round-robin scheduler policy core | ✅ Host-tested | `kernel/sched/sched_core.c`, `sched.c` (`tests/test_sched.c`) |
| Cooperative multitasking + RAM filesystem + `int 0x80` syscalls | ✅ Boot-verified (CI) | `kernel/sched/coop.c`, `kernel/fs/fs.c`, `kernel/arch/x86/syscall.c` |
| User mode (ring 3): TSS + `iret` to CPL 3 + `int 0x80` from userspace | ✅ Host-tested + boot-verified (CI) | `kernel/arch/x86/usermode.c`, `usermode_entry.S`, `tss.c` (`tests/test_usermode.c`) |
| **Modern desktop: 1024×768×32 framebuffer (Bochs VBE) + graphical terminal** | ✅ Boot-verified (CI) + screenshot | `kernel/drivers/lfb.c`, `kernel/gui/gconsole.c`, `desktop.c` |
| Boot in QEMU → `thuos>` (serial smoke-test) | ✅ Boot-verified (CI) | `scripts/boottest.sh`, CI `boot-smoke` |

> **Honesty:** logic is **host-tested** (`make test`: PMM, heap, paging, scheduler)
> and the kernel is **boot-verified in QEMU** by CI (`make boottest` → reaches
> `thuos>` over serial). It has **not** been run on physical hardware, and this dev
> sandbox has no QEMU (so `make boottest` skips here; CI runs it for real). See
> [`docs/THUOS_REALITY_CHECK.md`](docs/THUOS_REALITY_CHECK.md) for the honest big picture.

## Direction & what's next

THUOS's honest strategy — it cannot out-engineer Windows/macOS head-on (see
[`docs/THUOS_REALITY_CHECK.md`](docs/THUOS_REALITY_CHECK.md)) — is a focused wedge:
a **capability-secured, local-first, fast-boot developer/AI-agent OS that runs in
a VM/microVM** (sidestepping the driver moat). See
[`docs/THUOS_VISION.md`](docs/THUOS_VISION.md) and
[`docs/THUOS_ARCHITECTURE.md`](docs/THUOS_ARCHITECTURE.md).

Done so far (boot-verified in QEMU): paging enabled · context switch · cooperative
multitasking · RAM filesystem · `int 0x80` syscalls · ring 3 (user mode) ·
**a modern 1024×768 truecolor THU Desktop (Aurora) with the shell in a window**.
Next (staged — need boot-verification first): PS/2 mouse + clickable windows ·
per-process memory isolation · ELF loader + first userspace program.

---

## Quick start

```bash
make kernel     # build build/kernel.elf (freestanding 32-bit)
make verify     # ELF class, Multiboot header+checksum, symbols, allocator test -> BUILD_VERIFICATION.txt
make test       # host unit tests: PMM + kernel heap + paging + scheduler (native gcc)
make boottest   # BOOT THUOS in QEMU and verify it reaches thuos> (skips if no QEMU)
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
| [09_MEMORY_FOUNDATION](docs/09_MEMORY_FOUNDATION.md) | Physical memory manager (implemented) |
| [10_PAGING_PLAN](docs/10_PAGING_PLAN.md) | Paging design (planned) |
| [11_KERNEL_HEAP_PLAN](docs/11_KERNEL_HEAP_PLAN.md) | Kernel heap design (planned) |
| [THUOS_VISION](docs/THUOS_VISION.md) | Project vision & the winnable wedge |
| [THUOS_ARCHITECTURE](docs/THUOS_ARCHITECTURE.md) | Studied Linux/XNU/NT/seL4 → THUOS design |
| [THUOS_REALITY_CHECK](docs/THUOS_REALITY_CHECK.md) | Deep research: can THUOS rival the giants? |
| [BOOT_THUOS](BOOT_THUOS.md) | How to boot THUOS yourself (QEMU/ISO) |
| [PROJECT_STATUS](PROJECT_STATUS.md) | Subsystem status board |

## Principles

Local-first · privacy-first · honest engineering · emulator-first development ·
no destructive disk writes · no faked results. THUOS keeps the boot path working
after every milestone and never claims a build, boot, or install that did not
actually happen.
