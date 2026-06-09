# Changelog

All notable changes to THUOS are documented here. Format loosely follows
[Keep a Changelog](https://keepachangelog.com/). Versions track the THU Kernel.

## [0.16.0] — "Polish" — 2026-06-09

**Visual refinement.** The desktop gets a nicer look and, in particular, **real
pictogram app icons** instead of letters: a terminal prompt, a calculator face, a
folder, settings sliders, and an info badge — each on its own coloured tile. The
top bar gains a **live clock** (mm:ss) and the dock shows a **running-app
indicator** dot under the active app. The wallpaper, window and dock chrome were
tidied up.

### Added — kernel (graphics)
- `kernel/drivers/lfb.c` — `lfb_disc()` (filled circle) and `lfb_line()`
  (Bresenham), used to draw the pictogram icons.
- `kernel/gui/desktop.c` — `icon_glyph()` draws each app's pictogram; `draw_dock()`
  paints the dock + active indicator; `draw_clock()` updates each second in the
  event loop. Refined wallpaper gradient, window and title bar.

### Verification
- `make test` 8 host suites, `make verify` 16/0, no warnings. Boot-smoke still
  asserts `THU Desktop` + `thuos>` over serial. BOOT-VERIFIED in QEMU; the new
  look is verified by QEMU screenshots. No behaviour change to the apps.

## [0.15.0] — "Apps" — 2026-06-09

**A usable desktop.** THUOS gains a **PS/2 mouse** with a composited arrow
cursor, a **clickable dock**, and built-in **apps** that run in the window: a
**Terminal** (the shell), a working **Calculator**, a **Files** browser over the
RAM filesystem, an honest **System / Devices** panel, and **About**. Click a dock
icon to switch apps; `Esc` returns to the terminal (apps are also reachable by
the `calc`/`files`/`devices`/`about` commands).

Honest scope: this answers "basic apps that run here" for the apps that are
actually possible on a from-scratch OS. The **System** panel reports devices
truthfully — Display/Keyboard/Mouse/Storage/Timer **OK**; networking is an
*emulated NIC with no driver yet*; and **Camera, Wi-Fi and Bluetooth are
not supported** (no device / would need vendor firmware + drivers far beyond a
solo from-scratch OS). Nothing is faked. "Installing apps" (loading programs
from files via an ELF loader) is a later milestone.

### Added — kernel (desktop apps)
- `kernel/drivers/mouse.{c,h}` — PS/2 mouse (IRQ12): 8042 setup, 3-byte packet
  decode, clamped cursor position + buttons.
- `kernel/gui/apps.{c,h}` — Calculator (button grid + keyboard, integer math),
  Files (ramfs list + viewer), System/Devices (honest status), About.
- `kernel/gui/desktop.c` — a real event loop (`desktop_run`): composited mouse
  cursor (save/restore via `lfb_blit_*`), clickable dock, per-app keyboard/mouse
  routing, and the terminal as one app. Replaces the old blocking shell loop as
  the top level (text-mode `shell_run` kept as a fallback).
- Shell refactored into `shell_start` + `shell_feed_char`; non-blocking
  `keyboard_haskey`/`keyboard_trygetchar`; `lfb_blit_get/put`; `gcon_set_active`.

### Verification
- `make test` 8 host suites, `make verify` 16/0. Boot-smoke still asserts the
  `THU Desktop` marker and reaches `thuos>` over serial (the terminal is the
  default app). BOOT-VERIFIED in QEMU; the desktop, cursor, dock-click and apps
  are verified by QEMU screenshots (incl. a scripted dock click opening an app).

## [0.14.0] — "Aurora" — 2026-06-09

**A modern desktop.** THUOS moves off the 320×200 retro VGA mode onto a real
**high-resolution truecolour framebuffer — 1024×768×32** — via the Bochs VBE /
DISPI interface that QEMU's standard VGA emulates. The THU Desktop is redrawn in
a flat dark theme: a gradient wallpaper, a top bar, a shadowed rounded window
with traffic-light controls hosting the terminal, and a dock. The interactive
shell runs inside the terminal panel in scaled, crisp text.

No GRUB needed: the framebuffer is located by **PCI enumeration** (display-class
BAR0), mapped into paging, and the mode is set by programming the DISPI
registers directly. Serial is unchanged, so the boot self-test still verifies
the whole bring-up over COM1.

### Added — kernel (modern graphics)
- `kernel/drivers/lfb.{c,h}` — linear-framebuffer driver: PCI probe for the VGA
  BAR, DISPI mode-set (1024×768×32), and truecolour primitives (gradient, filled
  rounded rectangles, scaled text from the VGA font, fast rect scroll).
- `kernel/mm/vmm.c` — `vmm_map_lfb()` identity-maps the framebuffer's high MMIO
  window (a PCI BAR far above low RAM) with dedicated page tables.
- `kernel/arch/x86/io.h` — `outl`/`inl` for 32-bit PCI config access.
- `gfx.{c,h}` reduced to just the VGA font extraction; `gui/gconsole.c` and
  `gui/desktop.c` reworked for the 32bpp framebuffer (RGB colours, scaled glyphs,
  a flat dark desktop with a dock).

### Verification
- `make test` stays at 8 host suites; `make verify` 16/0. The `THU Desktop` boot
  marker still asserts over serial in CI `boot-smoke` (the kernel brings up the
  framebuffer and still reaches `thuos>`). BOOT-VERIFIED in QEMU; the desktop
  itself is verified by QEMU screenshot (1024×768 truecolour).

## [0.13.0] — "Desktop" — 2026-06-09

**THUOS gets a screen.** The kernel now switches the VGA into a real graphical
mode (mode 13h: 320×200, 256 colours, linear framebuffer at `0xA0000`) and draws
the **THU Desktop** — a top bar, a window with a title bar and traffic-light
controls, and a taskbar — with the interactive shell running as a graphical
terminal *inside* the window. The OS is no longer text-only.

No GRUB/BIOS dependency: the VGA registers are programmed directly, so it works
under QEMU's emulated VGA with the `-kernel` boot path. Serial output is
unchanged, so the boot self-test still verifies the whole bring-up over COM1.

### Added — kernel (graphics / desktop)
- `kernel/drivers/gfx.{c,h}` — mode 13h bring-up (register programming + DAC
  palette), pixel/rect/line primitives, and 8×16 text. The font is copied out of
  the VGA character generator (plane 2) at boot, so glyphs match the hardware and
  nothing has to be embedded.
- `kernel/gui/gconsole.{c,h}` — graphical text console: cursor, line wrap and
  scrolling, rendered into a framebuffer rectangle. `kputc()` routes here when
  graphics is active (serial mirroring is untouched).
- `kernel/gui/desktop.{c,h}` — the THU Desktop chrome and layout; hands its
  terminal region to the console.
- `kernel_main` extracts the font, prints the `THU Desktop` marker, switches to
  graphics, and runs the shell in the desktop terminal. Shell gains `gui`
  (repaint) and `clear` now clears the graphical console.

### Verification
- `scripts/boottest.sh` adds the `THU Desktop` boot marker; CI `boot-smoke`
  proves the kernel enters graphics mode and still reaches `thuos>` over serial.
  BOOT-VERIFIED in QEMU. The desktop itself is verified by QEMU screenshot
  (mode 13h is hard to assert over serial); `make test` stays at 8 host suites.

## [0.12.0] — "User Mode" — 2026-06-09

**Ring 3.** The kernel now drops the CPU to privilege level 3, runs code there,
and that code can re-enter the kernel **only** through the `int 0x80` syscall
gate — then control returns cleanly to ring 0. This is the privilege mechanism
every real OS is built on: a Task State Segment supplies the kernel stack for the
ring 3 → ring 0 transition, and an `iret` performs the ring 0 → ring 3 drop.

Honesty: the address space is still a single flat identity map (now marked
user-accessible), so this is **not** per-process memory isolation — that is a
later milestone. What is demonstrated is the CPL 0→3→0 transition and a real
syscall issued from user mode. **Boot-verified in QEMU** — the `boot-smoke`
serial log shows `[ring3] hello from CPL 3`, `last int 0x80 came from CS=0x1b =>
CPL 3`, the asserted `User mode` marker, and the kernel still reaching `thuos>`.

### Added — kernel (user mode / ring 3)
- `kernel/arch/x86/usermode_core.{c,h}` — pure descriptor/selector math (segment
  selectors, ring-3 `EFLAGS`, the 8-byte available-32-bit-TSS descriptor), free
  of kernel deps → host-tested.
- `kernel/arch/x86/tss.{c,h}` — the Task State Segment: holds `SS0:ESP0` (a
  dedicated kernel stack) that the CPU loads on every ring 3 → ring 0 entry. The
  GDT grows a sixth descriptor for it (`gdt.c`) and `ltr` loads the task register.
- `kernel/arch/x86/usermode_entry.S` — `usermode_enter()` builds an `iret` frame
  and drops to CPL 3; `usermode_resume_kernel()` is the one-way unwind the
  `SYS_EXIT` handler uses to come back; `tss_flush()` runs `ltr`.
- `kernel/arch/x86/usermode.c` — a ring-3 demo task that prints via `SYS_WRITE`,
  calls `SYS_GETPID`, then `SYS_EXIT`; `usermode_selftest()` runs it at boot.
- `SYS_EXIT` syscall; `syscall_last_cs()` records the caller's `CS` so `(cs & 3)`
  proves the trap really came from ring 3.
- `kernel/mm/vmm.c` — the identity map is now `PTE_RW | PTE_USER`, making the low
  8 MiB reachable from ring 3 (ring 0 is unaffected).
- Shell gains `user`: enter ring 3 and round-trip a syscall live, reporting CPL.

### Verification
- `tests/test_usermode.c` (+ `make test`, now 8 host suites): selector
  composition, ring-3 `EFLAGS` (IF set / NT cleared), TSS descriptor byte layout.
- `scripts/boottest.sh` asserts the `User mode` boot marker, so CI proves the
  kernel actually enters CPL 3, takes a syscall from there, and returns — then
  still reaches `thuos>`. BOOT-VERIFIED in QEMU (`boot-smoke`).

## [0.11.0] — "Syscalls" — 2026-06-06

A system-call interface (`int 0x80`) — the ABI boundary that userspace will use.
For now it is exercised from the kernel (ring 0); the same DPL-3 gate will serve
ring-3 processes once they exist. **Boot-verified in QEMU.**

### Added — kernel (syscalls)
- `kernel/arch/x86/syscall_core.{c,h}` — pure dispatch table (register/route,
  unknown → `ENOSYS`, bounds), free of kernel deps → host-tested.
- `kernel/arch/x86/syscall_stub.S` — `int 0x80` entry building the shared
  `registers_t` frame; the result is written to the saved `eax` (restored by
  `popa`), so the caller gets it in `eax` (number in eax, args in ebx/ecx/edx).
- `kernel/arch/x86/syscall.c` — handlers `SYS_UPTIME`, `SYS_WRITE`, `SYS_GETPID`,
  `SYS_VERSION`; installs the gate (DPL 3); `syscall_invoke()` for callers.
- `kernel_main` runs a syscall self-test at boot (real `int 0x80`); shell gains `sys`.

### Verification
- `tests/test_syscall.c` (+ `make test`, now 7 host suites): routing, ENOSYS,
  bounds, overwrite. `scripts/boottest.sh` asserts the `Syscall interface` marker
  — so CI proves `int 0x80` actually traps, dispatches, and returns. BOOT-VERIFIED.

## [0.10.0] — "Filesystem" — 2026-06-06

A real, usable **in-RAM filesystem** — the local-first storage foundation from
the strategy in `THUOS_VISION.md`. Sequenced ahead of preemptive multitasking
because it is host-testable (fits the verify-everything discipline) and higher
value for THUOS's wedge.

### Added — kernel (filesystem)
- `kernel/fs/ramfs_core.{c,h}` — pure file table over a caller-provided entry
  array + byte arena: create-or-replace `write`, `read`, `find`, listing,
  bump-allocation with an out-of-space guard. No kernel deps -> host-tested.
- `kernel/fs/fs.{c,h}` — static kernel filesystem (32 files, 64 KiB arena) seeded
  at boot with `welcome.txt` and `readme`.
- Shell: `ls`, `cat <file>`, `write <file> <text>` — create and read real files
  live in the booted kernel.

### Verification
- `tests/test_fs.c` (+ `make test`, now 6 host suites): create/write/read,
  create-or-replace, listing, arena-exhaustion guard.
- `scripts/boottest.sh` asserts the `RAM filesystem` boot marker. BOOT-VERIFIED in QEMU.

## [0.9.0] — "Cooperative Tasks" — 2026-06-06

Real **multitasking**: several kernel tasks run concurrently, each on its own
stack, handing off through the round-robin scheduler — combining the host-tested
scheduler policy (0.6) with the boot-verified context switch (0.8). **Boot-verified.**

### Added — kernel (multitasking)
- `kernel/sched/coop.{c,h}` — `coop_yield()` asks `sched_pick_next` who runs next
  and `thuos_context_switch`es to it; tasks loop, print, yield, then exit; once
  none are runnable, control returns to the kernel main/shell context.
- `kernel_main` runs the demo at boot (3 tasks, round-robin, to completion); shell
  gains `tasks` to run it live.

### Verification
- `scripts/boottest.sh` now asserts the `all tasks finished` marker, so CI proves
  the tasks actually ran via the scheduler, switched stacks, exited, and returned
  to main — then the kernel still reaches `thuos>`. BOOT-VERIFIED in QEMU.

## [0.8.0] — "Context Switch" — 2026-06-06

A real cooperative **context switch** — the primitive that makes processes
possible — running under the now-enabled paging, and **boot-verified in QEMU**.

### Added — kernel (tasks)
- `kernel/sched/context.S` — `thuos_context_switch(save_old_esp, new_esp)`:
  saves callee-saved registers, swaps the stack, restores and returns into the
  target context.
- `kernel/sched/task.{c,h}` — `task_init` builds a fresh task's initial stack so
  the first switch-in begins in `entry()` on its own stack. Pure setup,
  host-tested.
- `kernel_main` runs a context-switch self-test at boot: it switches into a task
  on a separate 4 KiB stack, which prints and switches back. If the switch were
  wrong the kernel would crash before the shell.

### Verification
- `tests/test_task.c` (+ `make test`, now 5 host suites) checks the initial
  stack frame. `scripts/boottest.sh` asserts the `Context switch OK` marker, so
  CI proves the switch works on real (emulated) hardware. BOOT-VERIFIED in QEMU.

## [0.7.0] — "Virtual Memory" — 2026-06-06

Paging is now **actually enabled** — the kernel runs under virtual memory, and
it's **boot-verified in QEMU** (not just a staged claim). This is the first of
the "risky staged" steps proven by the CI boot-smoke job: if enabling CR0.PG had
faulted, the kernel would never reach `thuos>` and CI would go red.

### Changed — kernel (virtual memory)
- `kernel/mm/vmm.c` — `vmm_enable()` is no longer gated/experimental: it loads
  CR3 with the page directory and sets `CR0.PG`. The low-8-MiB identity map
  covers the whole kernel image, stack, BSS (heap arena + PMM bitmap + the page
  tables) and VGA, so execution continues seamlessly under paging.
- `kernel_main` builds the tables then **enables paging** before starting the
  scheduler; `vmm_is_enabled()` added; shell `vmm` and `status` report it on.

### Verification
- `scripts/boottest.sh` now also asserts the `Paging ENABLED` boot marker, so CI
  proves the kernel survives enabling paging and still reaches its shell.
- BOOT-VERIFIED in QEMU (CI). Not yet on physical hardware.

## [0.6.1] — "Boot-Verified" — 2026-06-06

THUOS now actually **BOOTS**, verified automatically. Until now every milestone
was COMPILE-ONLY / HOST-TESTED; this turns "boot" from a claim into a checked fact.

### Added — verification
- `scripts/boottest.sh` — boots the kernel in QEMU, captures COM1 serial, and
  asserts the boot markers (THUOS banner → PMM → kernel heap → scheduler →
  `thuos>` prompt). `make boottest` runs it (skips gracefully if QEMU absent).
- CI `boot-smoke` job installs QEMU and runs the boot test on **every push**.

### Status
- **BOOT-VERIFIED: yes — in QEMU via CI** (i386, `-kernel` multiboot). The CI log
  shows the full boot log and the kernel reaching its interactive `thuos>` shell
  prompt over serial. (Not yet run on physical hardware; the dev sandbox itself
  has no QEMU, so it skips there.)

## [0.6.0] — "Scheduler" — 2026-06-06

Milestone 0.6: a round-robin process scheduler **policy core** — the run queue
and selection logic that decides who runs next — informed by the scheduler
study in `docs/THUOS_ARCHITECTURE.md` (Linux EEVDF, Mach threads). The actual
CPU context switch (register/stack swap that makes tasks truly run) is the next
step and is **staged** until paging is boot-verified.

### Added — kernel (scheduling)
- `kernel/sched/sched_core.{c,h}` — pure scheduler policy: task states
  (READY/RUNNING/BLOCKED/ZOMBIE), `sched_add`, round-robin `sched_pick_next`,
  time-slice `sched_on_tick`, `block`/`unblock`/`exit`, runnable count. Free of
  kernel deps so it is unit-tested on the host.
- `kernel/sched/sched.{c,h}` — glue: in-kernel run queue + initial tasks; the
  real (host-tested) policy runs live via the shell.
- `kernel_main` initialises the scheduler at boot.

### Added — shell commands
- `ps` — list scheduler tasks + states + quantum/ran stats.
- `sched` — run the round-robin policy live and print the pick order.
- Shell command count: 25 -> **27**.

### Added — verification
- `tests/test_sched.c` + `make test`: round-robin fairness, block/unblock,
  time-slice expiry, exit. Host-tested (now 4 host test suites: pmm, kheap,
  vmm, sched).

### Honesty
- HOST-TESTED: scheduler policy. The CPU **context switch is NOT implemented**
  yet (staged); tasks are scheduling descriptors the policy rotates over until
  paging is boot-verified and a context switch can be verified. BOOT-VERIFIED: none.

## [0.5.0] — "Paging" — 2026-06-06

Milestone 0.5: x86 paging tables + address translation, informed by a study of
how Linux, macOS (XNU), Windows (NT) and seL4 build virtual memory
(`docs/THUOS_ARCHITECTURE.md`). **Staged, not enabled:** the page directory and
tables are constructed and inspectable, but CR0.PG is not flipped because that
cannot be verified without QEMU here, and enabling an unverified mapping risks a
silent triple-fault.

### Added — kernel (virtual memory)
- `kernel/mm/vmm_core.{c,h}` — pure 2-level paging logic (PD/PT index split,
  PDE/PTE encode/decode, identity-map construction, address translation), free
  of kernel deps so it is unit-tested on the host.
- `kernel/mm/vmm.{c,h}` — glue: builds a 4 KiB-aligned page directory + page
  tables that identity-map the low 8 MiB; exposes translate/inspect helpers.
  `vmm_enable()` (load CR3 + set CR0.PG) is compiled in only under
  `THUOS_ENABLE_PAGING_EXPERIMENTAL` and must be boot-verified before use.
- `kernel_main` builds the tables at boot (staged); shell gains `vmm [addr]`.

### Added — docs & verification
- `docs/THUOS_ARCHITECTURE.md` — comparative study of Linux/XNU/NT/seL4 and a
  THUOS architecture (capability-secured ramping hybrid, local-first, honest).
- `tests/test_vmm.c` + `make test`: address split, entry encode/decode,
  identity map, translate==identity, unmapped→0, arbitrary va→pa. Host-tested.

### Honesty
- BOOT-VERIFIED: none here. Page-table logic is HOST-TESTED; paging enable is
  STAGED and unverified. Verify on QEMU/hardware via `BOOT_THUOS.md`.

## [0.4.0] — "Kernel Heap" — 2026-06-06

Milestone 0.4: a kernel heap (`kmalloc`/`kfree`). On top of the physical memory
manager, THUOS now has a general-purpose allocator so later subsystems (data
structures, processes, VFS) have somewhere to allocate from.

### Added — kernel (memory management)
- `kernel/mm/kheap_core.{c,h}` — pure free-list allocator core (address-ordered
  blocks, first-fit + splitting, forward coalescing, magic-guarded frees), free
  of kernel/hardware deps so it is unit-tested on the host.
- `kernel/mm/kheap.{c,h}` — kernel glue: `kmalloc`/`kcalloc`/`krealloc`/`kfree`
  backed by a static 1 MiB arena (Stage-A, pre-paging). The API stays stable
  when a paging-backed heap replaces the arena later.
- `kernel_main` initialises the heap right after the PMM and logs it at boot.

### Added — shell commands
- `heap` — kernel-heap statistics (total/used/free/blocks + integrity check).
- `kmalloc <bytes>` — allocate from the heap and print the pointer.
- `kfree <hex>` — free a `kmalloc` pointer.
- `status` now marks the kernel heap done and paging as the next (highest-risk) step.

### Added — verification, build & docs
- `tests/test_kheap.c` — host unit test: init/alloc/free, split, coalesce,
  realloc-preserve, exhaustion→NULL, double-free guard, full reclaim.
- `make test` now builds and runs both the PMM and kernel-heap host tests.
- `BOOT_THUOS.md` — how to build / boot THUOS in QEMU or on hardware and verify
  the boot yourself (honest: boot is COMPILE-ONLY in this dev environment).

### Honesty
- BOOT-VERIFIED: none here (no QEMU in the dev environment). The kernel links as
  an ELF Multiboot i386 image; boot is verifiable on a QEMU/hardware machine.

## [0.3.0] — "Memory Foundation" — 2026-06-05

Milestone 0.3: the physical memory manager. THUOS can now parse the Multiboot
memory map and hand out / reclaim physical memory one 4 KiB frame at a time,
with the protected regions reserved.

### Added — kernel (memory management)
- `kernel/mm/multiboot.h` — full Multiboot info + memory-map entry structures.
- `kernel/mm/frame_bitmap.{c,h}` — pure page-frame bitmap (1 bit/frame), free of
  kernel/hardware dependencies so it can be unit-tested on the host.
- `kernel/mm/pmm.{c,h}` — physical memory manager: parse the map, build a 4 GiB
  bitmap in `.bss`, mark `AVAILABLE` regions free, reserve protected regions,
  and allocate/free frames with statistics.
- Reserved regions: low 1 MiB (BIOS/IVT/EBDA/VGA), the kernel image
  (`thuos_kernel_start..end`, including the 16 KiB stack and the PMM bitmap),
  the Multiboot info structure, and the memory-map buffer.
- `pmm_alloc_frame` / `pmm_free_frame` (with protected-region refusal) and
  statistics: usable / reserved / free / used frames and usable bytes.
- `kernel_main` now calls `pmm_init` and prints memory statistics at boot.

### Added — shell commands
- `memmap` — dump the Multiboot memory map (base / length / type).
- `pages` — page-frame statistics.
- `allocpage` — allocate one 4 KiB physical frame and print its address.
- `freepage <hex>` — free a frame by physical address (refuses protected/unaligned).
- `mem` now reports the PMM summary; `sysinfo` shows usable memory + free frames.
- Shell command count: 17 → **21**.

### Added — verification & build
- `tests/test_pmm.c` — host unit test of the allocator core (native gcc, no QEMU):
  init/used/free, region reserve, alloc-skips-reserved, free+realloc reuse,
  exhaustion → `FB_NONE`, last-frame boundary.
- `make test` target; `make verify` now also checks `pmm_*` symbols and runs the
  allocator test. Verification: **16 checks passed, 0 failed**.

### Added — documentation
- `docs/09_MEMORY_FOUNDATION.md` (implemented PMM),
  `docs/10_PAGING_PLAN.md` and `docs/11_KERNEL_HEAP_PLAN.md` (design only).

### Changed
- Version bumped to 0.3.0 "Memory Foundation". `status`/`PROJECT_STATUS` updated.

### Not implemented (clearly planned)
- **Paging** and the **kernel heap** are design documents only (`docs/10`, `docs/11`),
  not code — they will be implemented when they can be boot-verified.

### Known limitations (honest)
- Still **not booted** here: `qemu-system-i386` is not installed in this
  environment, so the PMM's behavior on a real Multiboot map is unverified
  locally (the allocator core is, however, host unit-tested). No ISO built
  (`grub-mkrescue`/`xorriso` absent). No faked boot/ISO/screenshot results.

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
