/* THUOS — graphical text console.
 * Renders the kernel's character output into a rectangle of the mode-13h
 * framebuffer using the extracted 8x16 VGA font, with a cursor, line wrap and
 * scrolling. Once active, kputc() routes here instead of the text-mode VGA. */
#ifndef THUOS_GCONSOLE_H
#define THUOS_GCONSOLE_H

#include "types.h"

void gcon_init(int x, int y, int cols, int rows);  /* geometry in pixels/cells */
int  gcon_active(void);
void gcon_putc(char c);
void gcon_clear(void);
void gcon_set_color(uint8_t fg, int bg);            /* bg<0 keeps current bg */

#endif /* THUOS_GCONSOLE_H */
