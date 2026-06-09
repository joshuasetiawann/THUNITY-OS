# THUOS — Project Status

**Milestone:** 0.10 "Filesystem" — in-RAM local-first fs (ls/cat/write), host-tested + boot-verified in QEMU (CI)
**Date of this status:** 2026-06-06
**Honesty rule:** every "Implemented" item is backed by a source file and passes
the build + structural verification in [`BUILD_VERIFICATION.txt`](BUILD_VERIFICATION.txt).
A QEMU boot **is** now performed automatically in CI (`scripts/boottest.sh`): the
kernel boots and reaches its `thuos>` shell over serial. No claim here exceeds
what is actually run/tested.

## Status legend

- **Implemented** — code exists, compiles, links, and is wired into the kernel; structurally verified (and unit-tested where noted).
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
| IDT + CPU exceptions 0–31 | Implemented | `kernel/arch/x86/idt.c`, `isr.c`, `isr_stubs.S` |
| PIC remap + IRQ routing + EOI | Implemented | `kernel/arch/x86/pic.c`, `irq.c` |
| PIT timer @ 100 Hz + uptime | Implemented | `kernel/arch/x86/pit.c` |
| PS/2 keyboard (IRQ1) | Implemented | `kernel/drivers/keyboard.c` |
| Panic / assert | Implemented | `kernel/core/panic.c` |
| Shell (`thuos>`, 27 commands) | Implemented | `kernel/shell/shell.c` |
| **Scheduler (round-robin policy)** | **Implemented (policy core)** | `kernel/sched/sched_core.c`, `sched.c`; unit test `tests/test_sched.c`; context switch staged |
| **Boot in QEMU (serial smoke-test)** | **Boot-verified (CI)** | `scripts/boottest.sh`; CI `boot-smoke` job; reaches `thuos>` over COM1 |
| **Cooperative context switch** | **Boot-verified (QEMU/CI)** | `kernel/sched/context.S`, `task.c` (`tests/test_task.c`); boot asserts `Context switch OK` |
| **Cooperative multitasking** | **Boot-verified (QEMU/CI)** | `kernel/sched/coop.c` (scheduler + context switch); boot asserts `all tasks finished` |
| **RAM filesystem (ls/cat/write)** | **Host-tested + boot-verified** | `kernel/fs/ramfs_core.c`, `fs.c` (`tests/test_fs.c`); boot asserts `RAM filesystem` |
| Freestanding `mem*`/`str*` | Implemented | `kernel/lib/string.c` |
| **Multiboot memory-map parsing** | **Implemented** | `kernel/mm/multiboot.h`, `pmm.c` |
| **Physical memory manager (4 KiB frames)** | **Implemented** | `kernel/mm/pmm.c`, `frame_bitmap.c`; unit test `tests/test_pmm.c` |
| **Protected-region reservation** | **Implemented** | low 1 MiB + kernel image + Multiboot structs, in `pmm.c` |
| **Memory shell commands** (`memmap`,`pages`,`allocpage`,`freepage`) | **Implemented** | `kernel/shell/shell.c` |
| **Paging ENABLED (CR0.PG, identity map)** | **Boot-verified (QEMU/CI)** | `kernel/mm/vmm.c` enables paging; `vmm_core.c` host-tested (`tests/test_vmm.c`); boot-smoke asserts `Paging ENABLED` |
| **Kernel heap (`kmalloc`/`kfree`)** | **Implemented** | `kernel/mm/kheap_core.c`, `kheap.c`; unit test `tests/test_kheap.c` |
| VFS + initrd | Planned (0.4) | `docs/design/FILESYSTEM.md` |
| Userspace, syscalls, ELF loader | Planned (0.5) | `docs/08_ROADMAP.md` |
| Framebuffer graphics + mouse | Planned (0.6) | `docs/design/GUI_THU_DESKTOP.md` |
| In-kernel THU Desktop / window manager | Planned (0.7) | `docs/design/GUI_THU_DESKTOP.md` |
| THUFS filesystem | Designed | `docs/design/FILESYSTEM.md` |
| `thupkg` package manager | Designed | `docs/design/PACKAGE_MANAGER.md` |
| Security / sandboxing / signed packages | Designed | `docs/design/SECURITY_MODEL.md` |
| THU Desktop concept preview (web) | Implemented (preview only) | `preview/thuos_preview.html` |

## Verified in this environment

- `make clean && make kernel` — links `build/kernel.elf` with no warnings.
- `file build/kernel.elf` — *ELF 32-bit LSB executable, Intel 80386, statically linked*.
- Multiboot 1 header within the first 8 KiB; `magic + flags + checksum ≡ 0 (mod 2³²)`.
- Required symbols present: `_start`, `kernel_main`, `isr_handler`, `irq_handler`, `shell_run`, **`pmm_init`, `pmm_alloc_frame`, `pmm_free_frame`**.
- **`make test` — host unit test of the page-frame allocator passes** (init/reserve/alloc-skips-reserved/free+realloc/exhaustion/boundary).
- THU Desktop preview served over local HTTP (200 OK); its JavaScript passes `node --check`.
- Verification summary: **16 checks passed, 0 failed** (see `BUILD_VERIFICATION.txt`).

## NOT verified here (and why)

| Claim | Why not verified | How to verify elsewhere |
|-------|------------------|-------------------------|
| Kernel boots and the PMM runs on real memory | `qemu-system-i386` not installed in this environment | `sudo apt-get install qemu-system-x86` then `make run` |
| Bootable ISO builds | `grub-mkrescue` / `xorriso` not installed | `sudo apt-get install grub-pc-bin grub-common xorriso mtools` then `make iso` |

The PMM logic is exercised by a host unit test (no QEMU needed); its behavior on
an actual Multiboot memory map is observed only once booted. That distinction is
kept explicit.

## Next milestone

**THUOS 0.4 — Filesystem Foundation:** initrd/ramdisk, a VFS abstraction
(inode/file/dir/mount), and `ls`/`cat`/`pwd`. Paging and the kernel heap
(designed in `docs/10`, `docs/11`) are the other near-term increments, to be
implemented when they can be boot-verified.
