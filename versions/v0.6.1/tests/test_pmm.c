/* THUOS — host unit test for the page-frame allocator core.
 *
 * This compiles the SAME frame_bitmap.c the kernel uses (with THUOS_HOSTED_TEST
 * so it pulls <stdint.h> instead of the kernel's types.h) and exercises it with
 * plain assertions. It runs on the host with native gcc, so it needs no QEMU
 * and verifies the allocator logic that the kernel depends on.
 *
 * Build & run:  make test     (or: gcc -O2 tests/test_pmm.c -o build/test_pmm) */
#define THUOS_HOSTED_TEST
#include "../kernel/mm/frame_bitmap.c"

#include <stdio.h>
#include <assert.h>

int main(void) {
    enum { TOTAL = 8192 };          /* 8192 frames -> 1024-byte bitmap */
    static uint8_t bm[TOTAL / 8];

    /* 1) init marks everything USED */
    fb_init(bm, TOTAL);
    assert(fb_count_free(bm, TOTAL) == 0);
    assert(fb_alloc(bm, TOTAL) == FB_NONE);

    /* 2) free the whole space, then reserve the first 256 frames (1 MiB) */
    fb_mark_region_free(bm, TOTAL, 0, TOTAL);
    assert(fb_count_free(bm, TOTAL) == TOTAL);
    fb_mark_region_used(bm, TOTAL, 0, 256);
    assert(fb_count_free(bm, TOTAL) == TOTAL - 256);

    /* 3) first allocation must skip the reserved region -> frame 256 */
    uint32_t a = fb_alloc(bm, TOTAL);
    assert(a == 256 && fb_test(bm, 256) == 1);
    uint32_t b = fb_alloc(bm, TOTAL);
    assert(b == 257);

    /* 4) free + realloc reuses the lowest freed frame */
    fb_free(bm, a);
    assert(fb_test(bm, 256) == 0);
    uint32_t c = fb_alloc(bm, TOTAL);
    assert(c == 256);

    /* 5) exhaust all free frames, then expect FB_NONE */
    uint32_t free_now = fb_count_free(bm, TOTAL);
    for (uint32_t i = 0; i < free_now; i++) assert(fb_alloc(bm, TOTAL) != FB_NONE);
    assert(fb_count_free(bm, TOTAL) == 0);
    assert(fb_alloc(bm, TOTAL) == FB_NONE);

    /* 6) boundary bit math: set/clear the very last frame only */
    fb_mark_free(bm, TOTAL - 1);
    assert(fb_test(bm, TOTAL - 1) == 0);
    assert(fb_count_free(bm, TOTAL) == 1);
    assert(fb_alloc(bm, TOTAL) == TOTAL - 1);
    assert(fb_alloc(bm, TOTAL) == FB_NONE);

    printf("PMM allocator test: OK\n");
    printf("  init/used/free, region reserve, alloc-skips-reserved,\n");
    printf("  free+realloc reuse, exhaustion->FB_NONE, last-frame boundary\n");
    return 0;
}
