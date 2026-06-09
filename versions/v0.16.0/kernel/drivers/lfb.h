/* THUOS — high-resolution linear framebuffer (true colour).
 * Uses the Bochs VBE / DISPI interface that QEMU's std VGA emulates to set a
 * 32bpp linear mode (e.g. 1024x768), with the framebuffer found via PCI and
 * mapped into paging. This is the modern display path; the old VGA mode 13h
 * lives in gfx.c. Colours are 0x00RRGGBB. (Prefix lfb_ avoids the frame-bitmap
 * fb_ symbols in mm/.) */
#ifndef THUOS_LFB_H
#define THUOS_LFB_H

#include "types.h"

int  lfb_init(int w, int h);    /* probe + set mode + map LFB; 1 ok, 0 unavailable */
int  lfb_active(void);
int  lfb_width(void);
int  lfb_height(void);

void lfb_clear(uint32_t rgb);
void lfb_pixel(int x, int y, uint32_t rgb);
void lfb_fill(int x, int y, int w, int h, uint32_t rgb);
void lfb_gradient_v(int x, int y, int w, int h, uint32_t top, uint32_t bot);
void lfb_rect(int x, int y, int w, int h, uint32_t rgb);          /* 1px outline */
void lfb_round_fill(int x, int y, int w, int h, int r, uint32_t rgb);
void lfb_disc(int cx, int cy, int r, uint32_t rgb);              /* filled circle */
void lfb_line(int x0, int y0, int x1, int y1, uint32_t rgb);

/* 8x16 VGA glyphs, scaled by `s` (each font pixel -> s*s block). bg<0 = transparent. */
void lfb_char(int x, int y, char c, uint32_t fg, long bg, int s);
void lfb_text(int x, int y, const char *str, uint32_t fg, long bg, int s);

void lfb_scroll_up(int x, int y, int w, int h, int dy, uint32_t fill);

/* Save/restore a rectangle (used to composite a mouse cursor over the desktop). */
void lfb_blit_get(int x, int y, int w, int h, uint32_t *buf);
void lfb_blit_put(int x, int y, int w, int h, const uint32_t *buf);

#endif /* THUOS_LFB_H */
