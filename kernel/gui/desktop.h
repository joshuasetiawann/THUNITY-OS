/* THUOS — the THU Desktop: window manager + event loop hosting the apps. */
#ifndef THUOS_DESKTOP_H
#define THUOS_DESKTOP_H

enum { APP_TERMINAL = 0, APP_CALC, APP_FILES, APP_SYSTEM, APP_ABOUT };

void desktop_start(void);        /* enter graphics + draw the desktop chrome */
void desktop_draw(void);         /* (re)draw the desktop chrome */
void desktop_run(void);          /* mouse + dock + apps event loop (never returns) */
void desktop_open_app(int app);  /* switch the active app */

#endif /* THUOS_DESKTOP_H */
