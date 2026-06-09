/* THUOS — kernel heap glue.
 * Backs kmalloc/kfree with a single static 1 MiB arena (BSS) driven by the
 * pure allocator in kheap_core.c. This is honest about its stage: it is a
 * fixed arena that exists before paging. When paging + a virtual kernel heap
 * land, only this file changes; the kmalloc/kfree API stays the same. */
#include "kheap.h"
#include "kheap_core.h"

#define KHEAP_SIZE (1u << 20)   /* 1 MiB */

static uint8_t kheap_arena[KHEAP_SIZE];
static kheap_t kheap;

void kheap_init(void) {
    heap_init(&kheap, kheap_arena, KHEAP_SIZE);
}

void *kmalloc(size_t size)            { return heap_alloc(&kheap, size); }
void *krealloc(void *ptr, size_t s)   { return heap_realloc(&kheap, ptr, s); }
void  kfree(void *ptr)                { heap_free(&kheap, ptr); }

void *kcalloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    if (nmemb != 0 && total / nmemb != size) return NULL;   /* overflow */
    void *p = heap_alloc(&kheap, total);
    if (p) { uint8_t *b = (uint8_t *)p; for (size_t i = 0; i < total; i++) b[i] = 0; }
    return p;
}

size_t kheap_total(void)  { return heap_total(&kheap); }
size_t kheap_used(void)   { return heap_used(&kheap); }
size_t kheap_free(void)   { return heap_free_bytes(&kheap); }
size_t kheap_blocks(void) { return heap_blocks(&kheap); }
int    kheap_check(void)  { return heap_check(&kheap); }
