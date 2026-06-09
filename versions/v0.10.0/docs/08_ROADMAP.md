# 08 — Roadmap

**Status:** THUOS is at **Milestone 0.2 "Boot Seed"** (implemented). Everything
below 0.2 is implemented; 0.3 and beyond are planned. Milestones are honest
targets, not promises that any later work is done.

## Where we are

```
[x] 0.0.1  Boot Seed (baseline)   conception: Multiboot x86 kernel + shell direction
[x] 0.2.0  Boot Seed              kernel foundation, diagnostics, timer, preview
[x] 0.3.0  Memory Foundation      Multiboot map, 4 KiB frame allocator, mem cmds   <-- NOW
[ ] 0.4.0  Filesystem Foundation
[ ] 0.5.0  Userspace Seed
[ ] 0.6.0  Graphics Foundation
[ ] 0.7.0  THU Desktop Prototype
[ ] 1.0.0  Developer Preview ISO
```

## 0.2.0 — Boot Seed  ✅ (this milestone)

Multiboot boot, VGA console, serial COM1, `kprintf`, GDT, IDT, CPU exceptions
0–31, PIC remap, IRQ routing, PIT timer + uptime, PS/2 keyboard, panic/assert,
a 17-command `thuos>` shell, freestanding `mem*`/`str*`, a verifying build
system, and an interactive THU Desktop concept preview.

## 0.3.0 — Memory Foundation  ✅ (this milestone)

- Parse the Multiboot memory map. **done**
- Physical page (frame) allocator over 4 KiB pages. **done**
- Reserve low 1 MiB, kernel image, and Multiboot structures. **done**
- Memory shell commands: `memmap`, `pages`, `allocpage`, `freepage`. **done**
- Host unit test of the allocator (`make test`). **done**
- Identity paging + kernel VM layout — **designed**, not built ([`10_PAGING_PLAN.md`](10_PAGING_PLAN.md)).
- Kernel heap (`kmalloc`/`kfree`) — **designed**, not built ([`11_KERNEL_HEAP_PLAN.md`](11_KERNEL_HEAP_PLAN.md)).
- Doc: [`09_MEMORY_FOUNDATION.md`](09_MEMORY_FOUNDATION.md).

## 0.4.0 — Filesystem Foundation  ⏭ next

- Initrd / ramdisk loaded at boot.
- VFS abstraction (inode, file, directory, mountpoint).
- Devfs for device nodes; procfs-style introspection.
- Shell: `ls`, `cat`, `pwd`. THUFS design work. See `design/FILESYSTEM.md`.

## 0.5.0 — Userspace Seed

- Syscall table and ring-3 transition.
- ELF loader and an `init` process.
- Userland ABI and first core utilities.
- Cooperative then preemptive scheduler; process/thread model.

## 0.6.0 — Graphics Foundation

- Framebuffer mode (VBE/GOP) and drawing primitives.
- Bitmap font rendering; PS/2 mouse + cursor.
- A first compositor experiment.

## 0.7.0 — THU Desktop Prototype (in kernel)

- Window manager, terminal window, launcher/dock, settings, file manager —
  rendered by the kernel, guided by `preview/thuos_preview.html` and
  `design/GUI_THU_DESKTOP.md`.

## 1.0.0 — Developer Preview ISO

- Bootable ISO, stable shell, minimal filesystem, basic userspace, developer
  docs, a known-limitations list, and release notes.

## Long-term

Installer, persistent THUFS, signed `thupkg` packages and a real install
backend, a `.thuapp` app ecosystem, a security/sandboxing model, recovery mode,
and a safe update system. Specs: [`design/`](design/).

## Principles carried through every milestone

- The name stays **THUOS**; THUOS stays a from-scratch OS, not a Linux distro.
- Keep it bootable after every milestone; prefer small verified steps.
- Emulator-first; never write to real disks without explicit consent.
- Never fake a build, boot, screenshot, or install. Status stays honest.
