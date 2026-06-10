# 10 — Paging Plan

**Status: Planned / Designed — NOT implemented.** THU Kernel 0.3.0 runs with
paging **disabled** (flat segmentation only, physical = linear). This document is
the design for a future milestone. No code in the repo enables paging, and
nothing here should be read as working today.

> Why not implemented now: enabling paging changes every memory access and is
> only trustworthy once it can be **booted and observed** under an emulator.
> QEMU is unavailable in this build environment, so THUOS will not claim paging
> works until it can be boot-verified. The physical memory manager
> ([`09_MEMORY_FOUNDATION.md`](09_MEMORY_FOUNDATION.md)) is the prerequisite and
> is the part that is actually implemented now.

## Goal

Introduce 32-bit paging (4 KiB pages, optional 4 MiB large pages for the initial
identity map) so the kernel has a virtual address space, page-level protection,
and a foundation for userspace and a heap.

## Phase 1 — Identity map (bring-up safety)

Map virtual == physical for the regions the kernel already touches, so enabling
paging does not change any current pointer:

- `[0, end-of-RAM)` or at least `[0, 16 MiB)` identity-mapped initially.
- VGA at `0xB8000` must remain valid.
- The kernel image and stack must remain valid.

Mechanism: a page directory with 4 MiB pages (PSE) for the identity map is the
simplest first step; refine to 4 KiB page tables afterward.

## Phase 2 — Higher-half kernel (optional, later)

Move the kernel to a high virtual base (commonly `0xC0000000`) so userspace can
own the low address space:

```
0x00000000 ┌───────────────────────────┐
           │ user space (per process)  │
0xC0000000 ├───────────────────────────┤
           │ kernel virtual memory      │
           │  - kernel image (mapped)   │
           │  - kernel heap (see doc 11)│
           │  - temporary mappings      │
0xFFFFFFFF └───────────────────────────┘
```

This requires a boot trampoline that maps both the physical load address and the
high virtual address until `eip` is in the high half.

## Planned API (sketch — not present in code yet)

```c
void  paging_init(void);                          /* build dir, load CR3, set CR0.PG */
int   vmm_map(uint32_t virt, uint32_t phys, uint32_t flags);
int   vmm_unmap(uint32_t virt);
uint32_t vmm_translate(uint32_t virt);            /* virt -> phys, or 0 */
```

Page frames for directories/tables come from `pmm_alloc_frame()`.

## Interaction with the PMM

- Page tables and directories are allocated as physical frames via the PMM.
- The PMM already reserves the kernel image, low 1 MiB, and Multiboot structures,
  so identity-mapping those regions is safe.

## Verification plan (when implemented)

- Build still produces a valid Multiboot ELF (structural check, as today).
- Under QEMU: confirm the kernel survives `CR0.PG = 1`, the shell still runs, a
  test mapping/translation round-trips, and a deliberate access to an unmapped
  page raises **page fault (#14)** caught by the existing exception handler.
- A `crash pagefault` shell command may be added to exercise the fault path,
  mirroring the existing `crash div0`.

**Status summary:** Paging is **Planned**. The PMM it depends on is Implemented.
No paging code exists in 0.3.0; this is a design to be built and boot-verified
in a later milestone (target 0.3.x / 0.5 alongside userspace).
