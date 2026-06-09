# 11 — Kernel Heap Plan (`kmalloc` / `kfree`)

**Status: Planned / Designed — NOT implemented.** THU Kernel 0.3.0 has **no
heap**: there is no `kmalloc`/`kfree` in the kernel, and no code allocates
variable-sized memory. All kernel state is static or stack-based. This document
is the design for a future milestone.

> Why not implemented now: a robust heap is most valuable (and safest) once it
> sits on top of paging, and like paging it should be **boot-verified** before
> being called "working". QEMU is unavailable here, so THUOS will not ship a heap
> it cannot observe running. The physical frame allocator the heap will use
> *is* implemented and tested ([`09_MEMORY_FOUNDATION.md`](09_MEMORY_FOUNDATION.md)).

## Goal

Provide `kmalloc(size)` / `kfree(ptr)` for variable-sized kernel allocations
(driver buffers, future VFS nodes, process structures), backed by physical
frames from the PMM.

## Planned API (sketch — not present in code yet)

```c
void *kmalloc(size_t size);
void *kmalloc_aligned(size_t size, size_t align);
void  kfree(void *ptr);
void  kheap_stats(uint32_t *used, uint32_t *free);   /* for a `heap` shell cmd */
```

## Two-stage design

**Stage A — early bump allocator (no free).**
The simplest safe start: a watermark allocator over a region of frames obtained
from the PMM. `kmalloc` advances a pointer; `kfree` is a no-op (logged). Useful
for one-shot boot-time allocations and is trivially host-testable.

```
[ frame(s) from pmm_alloc_frame() ] -> [ used | used | ... | watermark -> free ]
```

**Stage B — free-list allocator.**
A real allocator with block headers (size + free flag), first-fit search,
splitting on allocation, and coalescing of adjacent free blocks on `kfree`. Grows
by requesting more frames from the PMM (and, once paging exists, mapping them
into the kernel heap window described in [`10_PAGING_PLAN.md`](10_PAGING_PLAN.md)).

```c
typedef struct block {
    uint32_t      size;     /* payload bytes            */
    int           free;     /* 1 = free                 */
    struct block *next;
} block_t;
```

## Interaction with the PMM and paging

- Heap memory is backed by `pmm_alloc_frame()`.
- Before paging exists, the heap can use a contiguous identity-mapped physical
  region. After paging exists, it uses the dedicated kernel heap virtual window.

## Verification plan (when implemented)

- A **host unit test** (same pattern as `tests/test_pmm.c`) for the free-list:
  alloc/free/realloc patterns, split + coalesce correctness, alignment, and
  out-of-memory behavior. This is fully verifiable without QEMU.
- In-kernel: a `heap` shell command reporting used/free bytes, plus a boot-time
  self-check, once it can be booted.

**Status summary:** Kernel heap is **Planned**. The PMM it will sit on is
Implemented and tested. No heap code exists in 0.3.0; Stage A (bump) is the next
safe, host-testable increment.
