/* THUOS — pure kernel-heap allocator core.
 * Deliberately free of any kernel/hardware dependency so the exact same code
 * can be unit-tested on the host (see tests/test_kheap.c). It manages an
 * explicit arena passed in by the caller using an intrusive, address-ordered
 * free list with block splitting and forward coalescing. */
#ifndef THUOS_KHEAP_CORE_H
#define THUOS_KHEAP_CORE_H

#ifdef THUOS_HOSTED_TEST
#include <stdint.h>
#include <stddef.h>
#else
#include "types.h"
#endif

struct kheap_block;   /* opaque block header, defined in kheap_core.c */

typedef struct kheap {
    uint8_t            *base;   /* arena start as given by the caller */
    size_t              size;   /* arena size in bytes */
    struct kheap_block *head;   /* first block in the address-ordered list */
} kheap_t;

void   heap_init(kheap_t *h, void *arena, size_t size);
void  *heap_alloc(kheap_t *h, size_t size);            /* NULL if no space */
void   heap_free(kheap_t *h, void *ptr);               /* NULL-safe, guarded */
void  *heap_realloc(kheap_t *h, void *ptr, size_t size);
size_t heap_total(const kheap_t *h);                   /* arena bytes */
size_t heap_used(const kheap_t *h);                    /* sum of used payloads */
size_t heap_free_bytes(const kheap_t *h);              /* sum of free payloads */
size_t heap_blocks(const kheap_t *h);                  /* block count */
int    heap_check(const kheap_t *h);                   /* 1 = consistent, 0 = corrupt */

#endif /* THUOS_KHEAP_CORE_H */
