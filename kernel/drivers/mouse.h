/* THUOS — PS/2 mouse driver (IRQ12).
 * Tracks an absolute cursor position (clamped to the screen) and button state
 * from the 3-byte PS/2 packets, so the desktop can have a pointer. */
#ifndef THUOS_MOUSE_H
#define THUOS_MOUSE_H

#include "types.h"

void mouse_init(int screen_w, int screen_h);
int  mouse_x(void);
int  mouse_y(void);
int  mouse_left(void);          /* 1 while the left button is down */
int  mouse_take_event(void);    /* 1 once if anything changed since the last call */
/* Apply a relative movement + button state from a USB HID mouse report
 * (HID convention: +dx right, +dy down; buttons bit0 left/1 right/2 middle). */
void mouse_inject(int dx, int dy, int buttons);

#endif /* THUOS_MOUSE_H */
