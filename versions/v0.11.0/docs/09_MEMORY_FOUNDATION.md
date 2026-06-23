# 09 — Memory Foundation (Physical Memory Manager)

**Status: Implemented and build-verified** in THU Kernel 0.3.0 "Memory
Foundation". The physical memory manager (PMM) compiles into the kernel, its
public symbols are present in the ELF, and its allocator core passes a host unit
test (`make test`). As with all of THUOS, "implemented" means it builds, links,
and is wired in — it has **not** been booted under QEMU here (no emulator in this
environment), so runtime behavior on real boot is not yet observed.

## What the PMM does

The PMM hands out and reclaims physical memory one **4 KiB page frame** at a
time. It is the foundation the future paging layer and kernel heap will build on.

Files:

| File | Role |
|------|------|
| `kernel/mm/multiboot.h` | Multiboot info + memory-map structures |
| `kernel/mm/frame_bitmap.{h,c}` | pure bitmap (1 bit/frame), no kernel deps — host-testable |
| `kernel/mm/pmm.{h,c}` | parse the map, reserve protected regions, alloc/free, stats |
| `tests/test_pmm.c` | host unit test of the allocator core |

## How it works

1. **Parse the Multiboot memory map.** `kernel_main` passes the Multiboot magic
   (`0x2BADB002`) and info pointer to `pmm_init`. If the `MMAP` flag is set, the
   PMM walks every `multiboot_mmap_entry_t` (each found at `ptr + size + 4`). If
   only the basic `MEM` flag is present, it falls back to assuming contiguous RAM
   from 1 MiB up to `mem_upper`.

2. **Bitmap allocation.** A static bitmap in `.bss` covers up to 4 GiB
   (`MAX_FRAMES = 0x100000` frames → 128 KiB). A **set bit = USED**, a
   **clear bit = FREE**. The bitmap starts all-USED; every `AVAILABLE` region
   from the map is then marked FREE. Because the bitmap lives inside the kernel
   image, it is covered by the kernel-image reservation automatically.

3. **Reserve protected regions** (flipped back to USED):
   - **Low 1 MiB** `[0, 0x100000)` — IVT, BDA, EBDA, VGA (`0xB8000`), BIOS.
   - **Kernel image** `[thuos_kernel_start, thuos_kernel_end)` — code, rodata,
     data, and `.bss` (which contains the 16 KiB stack and the PMM bitmap).
   - **Multiboot info structure** and the **memory-map buffer**.

   Each reserved range is also recorded so `freepage` can refuse to release it.

## Allocation API

```c
uint32_t pmm_alloc_frame(void);     /* physical address, or 0 if none free   */
int      pmm_free_frame(uint32_t a);/* 0 ok; -1 range, -2 protected, -3 free  */
```

Frame 0 is inside the protected low 1 MiB, so it is never handed out — which is
why `0` is an unambiguous "out of memory" return.

## Statistics

`pmm_print_stats()` (shell `pages`/`mem`) reports:

- **usable RAM** — total `AVAILABLE` bytes / frames from the map;
- **reserved** — usable frames held by protected ranges;
- **free** — frames currently free;
- **used** — reserved + allocated (`usable − free`).

## Shell commands (Milestone 0.3)

| Command | Effect |
|---------|--------|
| `memmap` | Dump the Multiboot memory map (base / length / type). |
| `pages` | Print frame statistics (usable / reserved / free / used). |
| `allocpage` | Allocate one 4 KiB frame and print its physical address. |
| `freepage <hex>` | Free a frame by physical address (refuses protected/unaligned). |
| `mem` | Memory hint + the statistics above. |

Example session (intended output, mirrored in the preview terminal):

```
thuos> pages
Physical memory (4 KiB frames):
  usable RAM : ~130048 KiB (32512 frames)
  reserved   : 318 frames (low 1 MiB + kernel + boot structs)
  free       : 32194 frames (~128776 KiB)
  used       : 318 frames (reserved + allocated)

thuos> allocpage
Allocated frame at physical 0x0012a000 (free now: 32193 frames)
Free it with: freepage 0x0012a000

thuos> freepage 0x0012a000
Freed frame at 0x0012a000 (free now: 32194 frames)
```

> The exact numbers depend on how much RAM the emulator/firmware reports; the
> values above are illustrative of the format, not a captured boot.

## Verification

```bash
make kernel     # builds with the PMM; 0 warnings
make verify     # checks ELF + Multiboot + symbols (incl. pmm_*) + runs the test
make test       # host unit test of the allocator core only
```

The host test (`tests/test_pmm.c`) compiles the *same* `frame_bitmap.c` the
kernel uses and asserts: init marks all used; region reserve/free; allocation
skips reserved frames; free-then-realloc reuses the lowest frame; exhaustion
returns `FB_NONE`; and last-frame boundary bit math. Results are recorded in
[`../BUILD_VERIFICATION.txt`](../BUILD_VERIFICATION.txt).

## Honest limitations

- 64-bit map addresses/lengths are printed as their low 32 bits (the 32-bit
  kernel only manages the first 4 GiB; regions are capped accordingly).
- This is *physical* frame management only. There is **no paging** and **no
  kernel heap** yet — see [`10_PAGING_PLAN.md`](10_PAGING_PLAN.md) and
  [`11_KERNEL_HEAP_PLAN.md`](11_KERNEL_HEAP_PLAN.md), which are designs, not code.

**Status summary:** PMM (parse map, 4 KiB frame bitmap, reservations, alloc/free,
stats, shell commands, host test) — **Implemented**. Paging & heap — **Planned**.
