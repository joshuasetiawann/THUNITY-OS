/* THUOS — VGA font extraction. See gfx.h.
 * The text-mode VGA keeps its 8x16 font in plane 2 of video memory; we copy it
 * into a private table at boot so the graphical framebuffer can reuse it. */
#include "gfx.h"
#include "io.h"

#define FB ((volatile uint8_t *)0xA0000)

static uint8_t font[256][16];
static int     font_ready = 0;

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
