/* THUOS — PS/2 keyboard driver (IRQ1, scancode set 1). */
#ifndef THUOS_KEYBOARD_H
#define THUOS_KEYBOARD_H

#include "types.h"

void keyboard_init(void);
/* Blocking read of one translated ASCII character (halts CPU while idle). */
char keyboard_getchar(void);

#endif /* THUOS_KEYBOARD_H */
