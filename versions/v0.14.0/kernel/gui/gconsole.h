/* THUOS — graphical text console over the high-res linear framebuffer.
 * Renders the kernel's character output into a rectangle of the framebuffer
 * using the scaled 8x16 VGA font, with a cursor, line wrap and scrolling. Once
 * active, kputc() routes here instead of the text-mode VGA. Colours are RGB. */
#ifndef THUOS_GCONSOLE_H
#define THUOS_GCONSOLE_H

#include "types.h"

void gcon_init(int x, int y, int cols, int rows, int scale, uint32_t fg, uint32_t bg);
int  gcon_active(void);
void gcon_putc(char c);
void gcon_clear(void);
void gcon_set_color(uint32_t fg, long bg);   /* bg<0 keeps current bg */

#endif /* THUOS_GCONSOLE_H */
