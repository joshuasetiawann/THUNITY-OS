/* THUOS — graphical text console (high-res framebuffer). See gconsole.h. */
#include "gconsole.h"
#include "lfb.h"

static int      active = 0;
static int      ox, oy, cols, rows, scale, cw, ch;   /* origin px, size cells, cell px */
static int      cx, cy;                               /* cursor (cells) */
static uint32_t fg = 0x9defb0, bg = 0x0b0e14;
static uint32_t accent = 0x6cc7ff;

static void cursor_draw(void)  { lfb_fill(ox + cx * cw, oy + cy * ch + ch - 2 * scale, cw, 2 * scale, accent); }
static void cursor_erase(void) { lfb_fill(ox + cx * cw, oy + cy * ch + ch - 2 * scale, cw, 2 * scale, bg); }

void gcon_init(int x, int y, int c, int r, int s, uint32_t f, uint32_t b) {
    ox = x; oy = y; cols = c; rows = r; scale = s;
    cw = 8 * s; ch = 16 * s;
    cx = cy = 0; fg = f; bg = b;
    lfb_fill(ox, oy, cols * cw, rows * ch, bg);
    active = 1;
    cursor_draw();
}

int  gcon_active(void) { return active; }
void gcon_set_active(int on) { active = on; }
void gcon_set_color(uint32_t f, long b) { fg = f; if (b >= 0) bg = (uint32_t)b; }

void gcon_clear(void) {
    lfb_fill(ox, oy, cols * cw, rows * ch, bg);
    cx = cy = 0;
    cursor_draw();
}

static void newline(void) {
    cx = 0;
    if (++cy >= rows) {
        lfb_scroll_up(ox, oy, cols * cw, rows * ch, ch, bg);
        cy = rows - 1;
    }
}

void gcon_putc(char c) {
    if (!active) return;
    cursor_erase();
    switch (c) {
        case '\n': newline(); break;
        case '\r': cx = 0; break;
        case '\t': cx = (cx + 4) & ~3; if (cx >= cols) newline(); break;
        case '\b':
            if (cx > 0) { cx--; lfb_char(ox + cx * cw, oy + cy * ch, ' ', fg, bg, scale); }
            break;
        default:
            lfb_char(ox + cx * cw, oy + cy * ch, c, fg, bg, scale);
            if (++cx >= cols) newline();
            break;
    }
    cursor_draw();
}
