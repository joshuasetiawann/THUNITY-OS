# 07 — Limitations and Next Steps

**Status:** Honest accounting of what is *not* done or *not* verified in THUOS
0.2.0, and exactly how to lift each limitation.

## Environment limitations (this build host)

The kernel was built and structurally verified here, but some tools are absent in
this environment, so a few things could **not** be verified locally. None of this
is hidden or worked around with fake output.

| Limitation | Impact | How to lift it |
|------------|--------|----------------|
| `qemu-system-i386` not installed | Kernel **boot is unverified** here | `sudo apt-get install qemu-system-x86` then `make run` |
| `grub-mkrescue` / `xorriso` not installed | No bootable **ISO** produced here | `sudo apt-get install grub-pc-bin grub-common xorriso mtools` then `make iso` |
| No headless browser (download blocked) | No **screenshot** of the preview | open `preview/thuos_preview.html` in any local browser |
| Hosted `gcc -m32` lacks 32-bit libc | Only affects *hosted* test programs, **not** this freestanding kernel | `sudo apt-get install gcc-multilib` (optional) |
| `nasm` not installed | None — boot/stubs use GNU `as` (`.S`) by design | n/a |

`make iso` and `make run` detect their missing tools and print the install
command instead of failing confusingly or pretending to succeed.

## Functional limitations (by design, this milestone)

THUOS 0.2.0 is a kernel *foundation*. It does **not** yet have:

- a memory manager (no `kmalloc`/paging beyond the loader's identity map);
- a filesystem or storage driver (no `ls`/`cat`/files yet);
- userspace, processes, scheduling, or syscalls (kernel runs in ring 0 only);
- graphics/framebuffer, mouse, or a real GUI (text mode only);
- networking, audio, or USB;
- a real `thupkg` install backend (the shell command is a design preview);
- an installer.

These are tracked in [`08_ROADMAP.md`](08_ROADMAP.md) and
[`../PROJECT_STATUS.md`](../PROJECT_STATUS.md), and several are specified in
[`design/`](design/).

## What you can verify yourself

```bash
make clean && make kernel && make verify   # build + structural verification
make status                                # one-screen status
make demo                                  # serve the preview, then curl/open it
node --check <(sed -n '/<script>/,/<\/script>/p' preview/thuos_preview.html)  # preview JS syntax
```

On a machine with QEMU you can also confirm the boot:

```bash
qemu-system-i386 -kernel build/kernel.elf -serial stdio -display none
# expect the [ OK ] bring-up lines and a 'thuos>' prompt
```

## Next steps — THUOS Milestone 0.3 (Memory Foundation)

1. Parse and display the Multiboot memory map safely.
2. Add a physical page (frame) allocator over 4 KiB pages; reserve the kernel
   image, boot structures, and VGA memory.
3. Add identity paging and a simple virtual-memory layout.
4. Add a kernel heap (`kmalloc`/`kfree`) with basic diagnostics.
5. Add shell commands: `memmap`, `pages`, `heap`.
6. Keep `make clean && make kernel && make verify` green; update docs and
   `PROJECT_STATUS.md`.

Rules carried forward: no libc in the kernel, no faked success, do not rewrite
working boot code, build after each step, and report exact verified commands.
