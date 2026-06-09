/* THUOS — VGA text-mode font access.
 * The character generator (8x16 glyphs) is copied out of VGA plane 2 at boot so
 * the high-res framebuffer (lfb.c) can render text that matches the hardware
 * font without embedding a table. Must be extracted while still in text mode. */
#ifndef THUOS_GFX_H
#define THUOS_GFX_H

#include "types.h"

void           vga_font_extract(void);  /* copy plane-2 font; call in text mode */
const uint8_t *gfx_glyph(uint8_t ch);   /* 16 bytes, MSB = leftmost pixel */
int            gfx_font_ready(void);

#endif /* THUOS_GFX_H */
