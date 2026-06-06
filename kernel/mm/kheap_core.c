/* THUOS — pure kernel-heap allocator core. No kernel dependencies.
 *
 * Layout: the arena is carved into a singly linked, address-ordered list of
 * blocks. Each block has a header followed by its payload. Allocation is
 * first-fit with splitting; freeing marks the block and coalesces adjacent
 * free blocks in a single forward pass, so freeing everything reclaims the
 * arena back to one block. A magic field guards against freeing foreign or
 * already-freed pointers. */
#include "kheap_core.h"

#define KH_MAGIC 0x4B484541u   /* "KHEA" */
#define KH_ALIGN 8u            /* payload alignment */

typedef struct kheap_block {
    size_t              size;   /* payload bytes (excludes this header) */
    struct kheap_block *next;   /* next block in address order, or NULL */
    uint32_t            free;   /* 1 = free, 0 = in use */
    uint32_t            magic;  /* KH_MAGIC */
} block_t;

#define HDR (sizeof(block_t))

static size_t    align_up(size_t v, size_t a)       { return (v + (a - 1)) & ~(a - 1); }
static uintptr_t ualign_up(uintptr_t v, uintptr_t a){ return (v + (a - 1)) & ~(a - 1); }

static void kh_memcpy(uint8_t *d, const uint8_t *s, size_t n) {
    for (size_t i = 0; i < n; i++) d[i] = s[i];
}

void heap_init(kheap_t *h, void *arena, size_t size) {
    uintptr_t raw   = (uintptr_t)arena;
    uintptr_t start = ualign_up(raw, KH_ALIGN);
    size_t    adj   = (size_t)(start - raw);

    h->base = (uint8_t *)arena;
    h->size = size;
    h->head = NULL;

    if (size < adj + HDR + KH_ALIGN) return;   /* too small to be useful */

    block_t *b = (block_t *)start;
    b->size  = size - adj - HDR;
    b->next  = NULL;
    b->free  = 1;
    b->magic = KH_MAGIC;
    h->head  = b;
}

void *heap_alloc(kheap_t *h, size_t size) {
    if (size == 0) return NULL;
    size = align_up(size, KH_ALIGN);

    for (block_t *b = h->head; b; b = b->next) {
        if (!b->free || b->size < size) continue;

        /* Split only if the remainder can hold a header + a minimum payload. */
        if (b->size >= size + HDR + KH_ALIGN) {
            block_t *nb = (block_t *)((uint8_t *)b + HDR + size);
            nb->size  = b->size - size - HDR;
            nb->next  = b->next;
            nb->free  = 1;
            nb->magic = KH_MAGIC;
            b->size = size;
            b->next = nb;
        }
        b->free = 0;
        return (uint8_t *)b + HDR;
    }
    return NULL;   /* out of heap memory */
}

static void coalesce(kheap_t *h) {
    for (block_t *b = h->head; b && b->next; ) {
        if (b->free && b->next->free) {
            b->size += HDR + b->next->size;
            b->next  = b->next->next;     /* keep b; it may merge again */
        } else {
            b = b->next;
        }
    }
}

void heap_free(kheap_t *h, void *ptr) {
    if (!ptr) return;
    block_t *b = (block_t *)((uint8_t *)ptr - HDR);
    if (b->magic != KH_MAGIC) return;   /* not one of ours / corrupt */
    if (b->free) return;                /* double-free guard */
    b->free = 1;
    coalesce(h);
}

void *heap_realloc(kheap_t *h, void *ptr, size_t size) {
    if (!ptr)      return heap_alloc(h, size);
    if (size == 0) { heap_free(h, ptr); return NULL; }

    block_t *b = (block_t *)((uint8_t *)ptr - HDR);
    if (b->magic != KH_MAGIC) return NULL;

    if (b->size >= align_up(size, KH_ALIGN)) return ptr;   /* fits in place */

    void *np = heap_alloc(h, size);
    if (!np) return NULL;
    kh_memcpy((uint8_t *)np, (const uint8_t *)ptr, b->size);
    heap_free(h, ptr);
    return np;
}

size_t heap_total(const kheap_t *h) { return h->size; }

size_t heap_used(const kheap_t *h) {
    size_t u = 0;
    for (const block_t *b = h->head; b; b = b->next) if (!b->free) u += b->size;
    return u;
}

size_t heap_free_bytes(const kheap_t *h) {
    size_t f = 0;
    for (const block_t *b = h->head; b; b = b->next) if (b->free) f += b->size;
    return f;
}

size_t heap_blocks(const kheap_t *h) {
    size_t c = 0;
    for (const block_t *b = h->head; b; b = b->next) c++;
    return c;
}

int heap_check(const kheap_t *h) {
    uintptr_t lo = (uintptr_t)h->base;
    uintptr_t hi = lo + h->size;
    for (const block_t *b = h->head; b; b = b->next) {
        if (b->magic != KH_MAGIC) return 0;
        uintptr_t s = (uintptr_t)b;
        uintptr_t e = s + HDR + b->size;
        if (s < lo || e > hi) return 0;                      /* inside arena */
        if (b->next && (uintptr_t)b->next != e) return 0;    /* contiguous */
    }
    return 1;
}
