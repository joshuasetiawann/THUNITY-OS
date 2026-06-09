/* THUOS — VGA mode 13h graphics. See gfx.h. */
#include "gfx.h"
#include "io.h"

#define FB ((volatile uint8_t *)0xA0000)

static uint8_t font[256][16];
static int     font_ready = 0;
static int     active = 0;

/* ---- font: copy the VGA character generator out of plane 2 (text mode) ---- */
void vga_font_extract(void) {
    /* save the sequencer/GC bits we touch */
    outb(0x3C4, 0x02); uint8_t s2 = inb(0x3C5);
    outb(0x3C4, 0x04); uint8_t s4 = inb(0x3C5);
    outb(0x3CE, 0x04); uint8_t g4 = inb(0x3CF);
    outb(0x3CE, 0x05); uint8_t g5 = inb(0x3CF);
    outb(0x3CE, 0x06); uint8_t g6 = inb(0x3CF);

    /* address plane 2 linearly at 0xA0000 */
    outb(0x3C4, 0x02); outb(0x3C5, 0x04);   /* map mask: plane 2          */
    outb(0x3C4, 0x04); outb(0x3C5, 0x06);   /* mem mode: ext, odd/even off */
    outb(0x3CE, 0x04); outb(0x3CF, 0x02);   /* read map select: plane 2    */
    outb(0x3CE, 0x05); outb(0x3CF, 0x00);   /* graphics mode: 0            */
    outb(0x3CE, 0x06); outb(0x3CF, 0x00);   /* misc: A0000 / 128K          */

    for (int c = 0; c < 256; c++)
        for (int r = 0; r < 16; r++)
            font[c][r] = FB[c * 32 + r];     /* 32-byte stride per glyph */

    /* restore */
    outb(0x3C4, 0x02); outb(0x3C5, s2);
    outb(0x3C4, 0x04); outb(0x3C5, s4);
    outb(0x3CE, 0x04); outb(0x3CF, g4);
    outb(0x3CE, 0x05); outb(0x3CF, g5);
    outb(0x3CE, 0x06); outb(0x3CF, g6);
    font_ready = 1;
}

const uint8_t *gfx_glyph(uint8_t ch) { return font[ch]; }
int            gfx_font_ready(void)  { return font_ready; }

/* ---- DAC palette ---- */
/* {r,g,b} in 6-bit (0..63). Index order matches the enum in gfx.h. */
static const uint8_t palette[][3] = {
    {0,0,0},{0,0,42},{0,42,0},{0,42,42},{42,0,0},{42,0,42},{42,21,0},{42,42,42},
    {21,21,21},{21,21,63},{21,63,21},{21,63,63},{63,21,21},{63,21,63},{63,63,21},{63,63,63},
    /* UI */
    {6,12,28},   /* DESK1   deep blue          */
    {10,18,40},  /* DESK2   gradient top       */
    {14,24,46},  /* TOPBAR                     */
    {8,14,30},   /* TASKBAR                    */
    {26,29,38},  /* WIN     window body        */
    {48,52,64},  /* WINBORDER                  */
    {18,52,60},  /* TITLE   title bar accent   */
    {60,60,63},  /* TITLETX                    */
    {3,7,12},    /* TERMBG  near-black         */
    {44,63,44},  /* TERMTX  phosphor green     */
    {20,60,63},  /* ACCENT  cyan               */
    {2,4,8},     /* SHADOW                     */
    {63,42,12},  /* BRAND   amber              */
    {34,40,50},  /* MUTED                      */
};

static void gfx_load_palette(void) {
    int n = (int)(sizeof palette / sizeof palette[0]);
    outb(0x3C8, 0);                 /* start at DAC index 0 */
    for (int i = 0; i < n; i++) {
        outb(0x3C9, palette[i][0]);
        outb(0x3C9, palette[i][1]);
        outb(0x3C9, palette[i][2]);
    }
}

/* ---- mode 13h register set ---- */
void gfx_enter(void) {
    static const uint8_t seq[5]  = { 0x03, 0x01, 0x0F, 0x00, 0x0E };
    static const uint8_t crtc[25] = {
        0x5F,0x4F,0x50,0x82,0x54,0x80,0xBF,0x1F,
        0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,
        0x9C,0x0E,0x8F,0x28,0x40,0x96,0xB9,0xA3,0xFF
    };
    static const uint8_t gc[9]   = { 0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0F,0xFF };
    static const uint8_t ac[21]  = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
        0x41,0x00,0x0F,0x00,0x00
    };

    outb(0x3C2, 0x63);                                  /* misc output */

    outb(0x3C4, 0x00); outb(0x3C5, 0x03);               /* seq: async reset */
    for (uint8_t i = 0; i < 5; i++) { outb(0x3C4, i); outb(0x3C5, seq[i]); }

    /* unlock CRTC indices 0-7 (clear write-protect in index 0x11) */
    outb(0x3D4, 0x11); outb(0x3D5, (uint8_t)(inb(0x3D5) & 0x7F));
    for (uint8_t i = 0; i < 25; i++) { outb(0x3D4, i); outb(0x3D5, crtc[i]); }

    for (uint8_t i = 0; i < 9; i++) { outb(0x3CE, i); outb(0x3CF, gc[i]); }

    (void)inb(0x3DA);                                   /* reset AC flip-flop */
    for (uint8_t i = 0; i < 21; i++) {
        (void)inb(0x3DA);
        outb(0x3C0, i);
        outb(0x3C0, ac[i]);
    }
    outb(0x3C0, 0x20);                                  /* re-enable video    */

    gfx_load_palette();
    active = 1;
    gfx_clear(COL_DESK1);
}

int gfx_active(void) { return active; }

/* ---- primitives ---- */
void gfx_pixel(int x, int y, uint8_t color) {
    if ((unsigned)x < GFX_W && (unsigned)y < GFX_H) FB[y * GFX_W + x] = color;
}

void gfx_clear(uint8_t color) {
    for (int i = 0; i < GFX_W * GFX_H; i++) FB[i] = color;
}

void gfx_fill(int x, int y, int w, int h, uint8_t color) {
    for (int j = 0; j < h; j++) {
        int yy = y + j;
        if ((unsigned)yy >= GFX_H) continue;
        for (int i = 0; i < w; i++) {
            int xx = x + i;
            if ((unsigned)xx < GFX_W) FB[yy * GFX_W + xx] = color;
        }
    }
}

void gfx_hline(int x, int y, int w, uint8_t color) { gfx_fill(x, y, w, 1, color); }

void gfx_rect(int x, int y, int w, int h, uint8_t color) {
    gfx_fill(x, y, w, 1, color);
    gfx_fill(x, y + h - 1, w, 1, color);
    gfx_fill(x, y, 1, h, color);
    gfx_fill(x + w - 1, y, 1, h, color);
}

void gfx_char(int x, int y, char c, uint8_t fg, int bg) {
    const uint8_t *g = font[(uint8_t)c];
    for (int r = 0; r < 16; r++) {
        uint8_t bits = g[r];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) gfx_pixel(x + col, y + r, fg);
            else if (bg >= 0)         gfx_pixel(x + col, y + r, (uint8_t)bg);
        }
    }
}

void gfx_text(int x, int y, const char *s, uint8_t fg, int bg) {
    for (; *s; s++, x += 8) gfx_char(x, y, *s, fg, bg);
}

void gfx_scroll_up(int x, int y, int w, int h, int dy, uint8_t fill) {
    for (int yy = 0; yy < h - dy; yy++) {
        volatile uint8_t *d = FB + (y + yy) * GFX_W + x;
        volatile uint8_t *s = FB + (y + yy + dy) * GFX_W + x;
        for (int xx = 0; xx < w; xx++) d[xx] = s[xx];
    }
    gfx_fill(x, y + h - dy, w, dy, fill);
}
