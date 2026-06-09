# THUOS — Version Archive (`master`)

A frozen archive of **every released version of THUOS**, one **top-level folder
per release** (`THUOS 0.2.0` … `THUOS 0.17.0`) — each a faithful snapshot of the
source tree at that version, taken from git history.

- The latest, live, **buildable** kernel is on the **[`main`](../../tree/main)**
  branch — build and run from there.
- This `master` branch only collects the historical snapshots for browsing; it
  is not built by CI.

| Version | Codename | What it added |
|---------|----------|---------------|
| THUOS 0.2.0  | Boot Seed         | Multiboot boot, VGA text, serial, GDT/IDT, exceptions, PIC/IRQ, PIT, keyboard, shell |
| THUOS 0.3.0  | Memory Foundation | Physical memory manager (4 KiB frame bitmap) |
| THUOS 0.4.0  | Kernel Heap       | `kmalloc`/`kfree` over a free-list arena |
| THUOS 0.5.0  | Paging            | x86 page tables + address translation (staged) |
| THUOS 0.6.0  | Scheduler         | Round-robin scheduler policy core |
| THUOS 0.6.1  | Boot-Verified     | First real QEMU boot smoke-test in CI |
| THUOS 0.7.0  | Virtual Memory    | Paging ENABLED (CR0.PG) |
| THUOS 0.8.0  | Context Switch    | Cooperative context switch |
| THUOS 0.9.0  | Cooperative Tasks | Multitasking via scheduler + context switch |
| THUOS 0.10.0 | Filesystem        | In-RAM filesystem (`ls`/`cat`/`write`) |
| THUOS 0.11.0 | Syscalls          | `int 0x80` syscall ABI |
| THUOS 0.12.0 | User Mode         | Ring 3: TSS + `iret` to CPL 3 + `int 0x80` from userspace |
| THUOS 0.13.0 | Desktop           | VGA graphics (mode 13h) + the THU Desktop + graphical terminal |
| THUOS 0.14.0 | Aurora            | High-res truecolor desktop (1024×768×32 via Bochs VBE) |
| THUOS 0.15.0 | Apps              | PS/2 mouse + clickable dock + apps (Terminal/Calculator/Files/System/About) |
| THUOS 0.16.0 | Polish            | Pictogram app icons, top-bar clock, active-app indicator |
| THUOS 0.17.0 | Suite             | Settings menu (live themes) + Notes + Paint |

> There is no `v0.1` source — `0.2.0 "Boot Seed"` is the first bootable kernel.
> Full per-release notes live in `CHANGELOG.md` inside any snapshot
> (e.g. `THUOS 0.17.0/CHANGELOG.md`).
