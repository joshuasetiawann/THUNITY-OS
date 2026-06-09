# THUOS — versioned snapshots

Each folder is a **faithful snapshot of the source tree at that release**, taken
from git history (`git archive <milestone-commit>`). It lets you browse exactly
what the OS was at every step, side by side.

- The **root of the repository is the live, latest, buildable tree** — that is
  what CI builds and boot-verifies on every push. Build/run from the root.
- Each release also gets a frozen folder here; new releases add a new folder.
- Snapshots are source only (no build artifacts). Older versions are kept as they
  were and are not maintained.

| Version | Codename | What it added |
|---------|----------|---------------|
| v0.2.0  | Boot Seed        | Multiboot boot stub, VGA text, serial, GDT/IDT, exceptions, PIC/IRQ, PIT, keyboard, shell |
| v0.3.0  | Memory Foundation| Physical memory manager (4 KiB frame bitmap) |
| v0.4.0  | Kernel Heap      | `kmalloc`/`kfree` over a free-list arena |
| v0.5.0  | Paging           | x86 page tables + address translation (staged) |
| v0.6.0  | Scheduler        | Round-robin scheduler policy core |
| v0.6.1  | Boot-Verified    | First real QEMU boot smoke-test in CI |
| v0.7.0  | Virtual Memory   | Paging ENABLED (CR0.PG), running under virtual memory |
| v0.8.0  | Context Switch   | Real cooperative context switch |
| v0.9.0  | Cooperative Tasks| Multitasking: 3 tasks via scheduler + context switch |
| v0.10.0 | Filesystem       | Local-first in-RAM filesystem (`ls`/`cat`/`write`) |
| v0.11.0 | Syscalls         | `int 0x80` syscall ABI (foundation for userspace) |
| v0.12.0 | User Mode        | Ring 3: TSS + `iret` to CPL 3 + `int 0x80` from userspace |
| v0.13.0 | Desktop          | VGA graphics (mode 13h) + the THU Desktop, shell in a graphical terminal |
| v0.14.0 | Aurora           | Modern high-res truecolor desktop (1024×768×32 via Bochs VBE), dark theme + dock |

> There is no `v0.1` source — `0.2.0 "Boot Seed"` is the first bootable kernel.
> Full per-release notes are in [`../CHANGELOG.md`](../CHANGELOG.md).

## Build any version

The root tree is the canonical build. To build a historical snapshot:

```bash
cd versions/v0.11.0
make kernel      # needs gcc-multilib (32-bit)
make boottest    # boots it in QEMU if qemu-system-x86 is installed
```
