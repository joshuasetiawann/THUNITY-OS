/* THUOS — pure page-frame bitmap implementation. No kernel dependencies. */
#include "frame_bitmap.h"

static uint32_t byte_index(uint32_t frame) { return frame >> 3; }
static uint8_t  bit_mask(uint32_t frame)   { return (uint8_t)(1u << (frame & 7u)); }

void fb_init(uint8_t *bm, uint32_t total_frames) {
    uint32_t bytes = (total_frames + 7u) / 8u;
    for (uint32_t i = 0; i < bytes; i++) bm[i] = 0xFFu;   /* all USED */
}

void fb_mark_used(uint8_t *bm, uint32_t frame) {
    bm[byte_index(frame)] |= bit_mask(frame);
}

void fb_mark_free(uint8_t *bm, uint32_t frame) {
    bm[byte_index(frame)] &= (uint8_t)~bit_mask(frame);
}

int fb_test(const uint8_t *bm, uint32_t frame) {
    return (bm[byte_index(frame)] & bit_mask(frame)) ? 1 : 0;
}

void fb_mark_region_used(uint8_t *bm, uint32_t total, uint32_t first, uint32_t count) {
    for (uint32_t i = 0; i < count && (first + i) < total; i++) fb_mark_used(bm, first + i);
}

void fb_mark_region_free(uint8_t *bm, uint32_t total, uint32_t first, uint32_t count) {
    for (uint32_t i = 0; i < count && (first + i) < total; i++) fb_mark_free(bm, first + i);
}

uint32_t fb_alloc(uint8_t *bm, uint32_t total_frames) {
    uint32_t bytes = (total_frames + 7u) / 8u;
    for (uint32_t b = 0; b < bytes; b++) {
        if (bm[b] == 0xFFu) continue;                 /* fully used byte, skip */
        for (uint32_t bit = 0; bit < 8u; bit++) {
            uint32_t frame = b * 8u + bit;
            if (frame >= total_frames) return FB_NONE;
            if (!(bm[b] & (1u << bit))) {
                bm[b] |= (uint8_t)(1u << bit);
                return frame;
            }
        }
    }
    return FB_NONE;
}

void fb_free(uint8_t *bm, uint32_t frame) {
    fb_mark_free(bm, frame);
}

uint32_t fb_count_free(const uint8_t *bm, uint32_t total_frames) {
    uint32_t count = 0;
    for (uint32_t f = 0; f < total_frames; f++) if (!fb_test(bm, f)) count++;
    return count;
}
