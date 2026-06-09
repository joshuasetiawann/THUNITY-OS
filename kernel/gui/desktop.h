/* THUOS — the THU Desktop: a graphical shell environment drawn by the kernel
 * (top bar, window with a title bar, a taskbar) hosting the terminal. */
#ifndef THUOS_DESKTOP_H
#define THUOS_DESKTOP_H

void desktop_start(void);   /* enter mode 13h, draw the desktop, start the console */
void desktop_draw(void);    /* (re)draw the desktop chrome + reset the terminal    */

#endif /* THUOS_DESKTOP_H */
