/* THUOS — PS/2 keyboard driver (IRQ1, scancode set 1). */
#ifndef THUOS_KEYBOARD_H
#define THUOS_KEYBOARD_H

#include "types.h"

void keyboard_init(void);
/* Blocking read of one translated ASCII character (halts CPU while idle). */
char keyboard_getchar(void);
int  keyboard_haskey(void);        /* non-blocking: 1 if a key is pending */
char keyboard_trygetchar(void);    /* non-blocking: next key, or 0 if none */

#endif /* THUOS_KEYBOARD_H */
