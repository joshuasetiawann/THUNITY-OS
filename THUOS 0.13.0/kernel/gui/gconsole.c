/* THUOS — graphical text console. See gconsole.h. */
#include "gconsole.h"
#include "gfx.h"

#define CW 8        /* glyph cell width  */
#define CH 16       /* glyph cell height */

static int     active = 0;
static int     ox, oy, cols, rows;   /* origin (px) + size (cells) */
static int     cx, cy;               /* cursor (cells)             */
static uint8_t fg = COL_TERMTX;
static int     bg = COL_TERMBG;

/* The cursor is a 2px underline drawn in an otherwise-empty cell, so erasing it
 * (refilling bg) never clips a real glyph. */
static void cursor_draw(void)  { gfx_fill(ox + cx * CW, oy + cy * CH + CH - 2, CW, 2, COL_ACCENT); }
static void cursor_erase(void) { gfx_fill(ox + cx * CW, oy + cy * CH + CH - 2, CW, 2, (uint8_t)bg); }

void gcon_init(int x, int y, int c, int r) {
    ox = x; oy = y; cols = c; rows = r; cx = cy = 0;
    fg = COL_TERMTX; bg = COL_TERMBG;
    gfx_fill(ox, oy, cols * CW, rows * CH, (uint8_t)bg);
    active = 1;
    cursor_draw();
}

int  gcon_active(void) { return active; }

void gcon_set_color(uint8_t f, int b) { fg = f; if (b >= 0) bg = b; }

void gcon_clear(void) {
    gfx_fill(ox, oy, cols * CW, rows * CH, (uint8_t)bg);
    cx = cy = 0;
    cursor_draw();
}

static void newline(void) {
    cx = 0;
    if (++cy >= rows) {
        gfx_scroll_up(ox, oy, cols * CW, rows * CH, CH, (uint8_t)bg);
        cy = rows - 1;
    }
}

void gcon_putc(char ch) {
    if (!active) return;
    cursor_erase();
    switch (ch) {
        case '\n': newline(); break;
        case '\r': cx = 0; break;
        case '\t': cx = (cx + 4) & ~3; if (cx >= cols) newline(); break;
        case '\b':
            if (cx > 0) { cx--; gfx_char(ox + cx * CW, oy + cy * CH, ' ', fg, bg); }
            break;
        default:
            gfx_char(ox + cx * CW, oy + cy * CH, ch, fg, bg);
            if (++cx >= cols) newline();
            break;
    }
    cursor_draw();
}
