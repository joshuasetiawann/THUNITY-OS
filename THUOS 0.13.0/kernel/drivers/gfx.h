/* THUOS — VGA graphics (mode 13h, 320x200x256 linear framebuffer at 0xA0000).
 * The first real pixel-addressable display for THUOS. Reliable under QEMU's
 * emulated VGA without a BIOS/GRUB: we program the VGA registers directly. */
#ifndef THUOS_GFX_H
#define THUOS_GFX_H

#include "types.h"

#define GFX_W 320
#define GFX_H 200

/* Named palette indices we program into the DAC (see gfx_init_palette). */
enum {
    COL_BLACK = 0, COL_BLUE, COL_GREEN, COL_CYAN, COL_RED, COL_MAGENTA,
    COL_BROWN, COL_LGREY, COL_DGREY, COL_LBLUE, COL_LGREEN, COL_LCYAN,
    COL_LRED, COL_LMAGENTA, COL_YELLOW, COL_WHITE,
    /* UI palette */
    COL_DESK1 = 16, COL_DESK2, COL_TOPBAR, COL_TASKBAR, COL_WIN, COL_WINBORDER,
    COL_TITLE, COL_TITLETX, COL_TERMBG, COL_TERMTX, COL_ACCENT, COL_SHADOW,
    COL_BRAND, COL_MUTED
};

/* Copy the VGA character generator (plane 2) into a private 8x16 font table.
 * MUST be called while still in text mode (before gfx_enter). */
void           vga_font_extract(void);
const uint8_t *gfx_glyph(uint8_t ch);   /* 16 bytes, MSB = leftmost pixel */
int            gfx_font_ready(void);

void gfx_enter(void);          /* switch the VGA to mode 13h + load our palette */
int  gfx_active(void);         /* 1 once gfx_enter has run                       */

/* Drawing primitives (bounds-checked). */
void gfx_clear(uint8_t color);
void gfx_pixel(int x, int y, uint8_t color);
void gfx_fill(int x, int y, int w, int h, uint8_t color);
void gfx_rect(int x, int y, int w, int h, uint8_t color);        /* outline */
void gfx_hline(int x, int y, int w, uint8_t color);
void gfx_char(int x, int y, char c, uint8_t fg, int bg);         /* bg<0 = transparent */
void gfx_text(int x, int y, const char *s, uint8_t fg, int bg);
void gfx_scroll_up(int x, int y, int w, int h, int dy, uint8_t fill);  /* scroll a rect up */

#endif /* THUOS_GFX_H */
