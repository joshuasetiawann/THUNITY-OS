/* THUOS — pure page-frame bitmap.
 * Deliberately free of any kernel/hardware dependency so the exact same code
 * can be unit-tested on the host (see tests/test_pmm.c). A set bit means the
 * frame is USED; a clear bit means it is FREE. */
#ifndef THUOS_FRAME_BITMAP_H
#define THUOS_FRAME_BITMAP_H

#ifdef THUOS_HOSTED_TEST
#include <stdint.h>
#include <stddef.h>
#else
#include "types.h"
#endif

#define FB_NONE 0xFFFFFFFFu   /* returned by fb_alloc when nothing is free */

void     fb_init(uint8_t *bm, uint32_t total_frames);          /* mark all USED */
void     fb_mark_used(uint8_t *bm, uint32_t frame);
void     fb_mark_free(uint8_t *bm, uint32_t frame);
int      fb_test(const uint8_t *bm, uint32_t frame);           /* 1 = used */
void     fb_mark_region_used(uint8_t *bm, uint32_t total, uint32_t first, uint32_t count);
void     fb_mark_region_free(uint8_t *bm, uint32_t total, uint32_t first, uint32_t count);
uint32_t fb_alloc(uint8_t *bm, uint32_t total_frames);         /* first free frame, or FB_NONE */
void     fb_free(uint8_t *bm, uint32_t frame);
uint32_t fb_count_free(const uint8_t *bm, uint32_t total_frames);

#endif /* THUOS_FRAME_BITMAP_H */
