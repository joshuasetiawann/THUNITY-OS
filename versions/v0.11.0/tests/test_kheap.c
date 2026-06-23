/* THUOS — host unit test for the kernel-heap allocator core.
 *
 * Compiles the SAME kheap_core.c the kernel uses (with THUOS_HOSTED_TEST so it
 * pulls <stdint.h>/<stddef.h> instead of the kernel's types.h) and exercises
 * it with plain assertions on the host. No QEMU required — this verifies the
 * allocator logic that kmalloc/kfree depend on.
 *
 * Build & run:  make test   (or: gcc -O2 tests/test_kheap.c -o build/test_kheap) */
#define THUOS_HOSTED_TEST
#include "../kernel/mm/kheap_core.c"

#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    static unsigned char arena[64 * 1024];
    kheap_t h;

    /* 1) init: one consistent free block spanning (almost) the whole arena */
    heap_init(&h, arena, sizeof arena);
    assert(heap_check(&h));
    assert(heap_blocks(&h) == 1);
    assert(heap_total(&h) == sizeof arena);
    size_t free0 = heap_free_bytes(&h);
    assert(free0 > 0 && heap_used(&h) == 0);

    /* 2) two distinct, non-overlapping allocations that are writable */
    unsigned char *a = heap_alloc(&h, 100);
    unsigned char *b = heap_alloc(&h, 200);
    assert(a && b && a != b);
    assert((a + 100 <= b) || (b + 200 <= a));     /* no overlap */
    memset(a, 0xAA, 100);
    memset(b, 0xBB, 200);
    assert(heap_used(&h) >= 300);
    assert(heap_check(&h));

    /* 3) free both + coalesce returns the arena to its original free size */
    heap_free(&h, a);
    heap_free(&h, b);
    assert(heap_check(&h));
    assert(heap_free_bytes(&h) == free0);

    /* 4) a request larger than the arena fails (returns NULL), heap intact */
    assert(heap_alloc(&h, sizeof arena) == NULL);
    assert(heap_check(&h));

    /* 5) realloc grows a block and preserves the existing bytes */
    unsigned char *p = heap_alloc(&h, 50);
    for (int i = 0; i < 50; i++) p[i] = (unsigned char)(i + 1);
    unsigned char *q = heap_realloc(&h, p, 500);
    assert(q);
    for (int i = 0; i < 50; i++) assert(q[i] == (unsigned char)(i + 1));
    heap_free(&h, q);
    assert(heap_free_bytes(&h) == free0);

    /* 6) double-free and foreign-pointer frees are ignored, not corrupting */
    unsigned char *r = heap_alloc(&h, 32);
    heap_free(&h, r);
    heap_free(&h, r);                 /* double free: guarded no-op */
    static unsigned char foreign[128];
    memset(foreign, 0, sizeof foreign);   /* its "magic" field reads as 0 */
    heap_free(&h, foreign + 64);      /* foreign pointer: magic won't match -> no-op */
    assert(heap_check(&h));
    assert(heap_free_bytes(&h) == free0);

    /* 7) many small allocations, freed in reverse, fully reclaim the arena */
    enum { N = 200 };
    unsigned char *v[N];
    for (int i = 0; i < N; i++) { v[i] = heap_alloc(&h, 16); assert(v[i]); }
    assert(heap_check(&h));
    for (int i = N - 1; i >= 0; i--) heap_free(&h, v[i]);
    assert(heap_check(&h));
    assert(heap_free_bytes(&h) == free0);
    assert(heap_blocks(&h) == 1);

    printf("kheap allocator test: OK\n");
    printf("  init/alloc/free, split, coalesce, realloc-preserve,\n");
    printf("  exhaustion->NULL, double-free guard, full reclaim\n");
    return 0;
}
