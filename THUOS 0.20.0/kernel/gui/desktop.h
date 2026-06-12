/* THUOS — the THU Desktop: window manager + event loop hosting the apps. */
#ifndef THUOS_DESKTOP_H
#define THUOS_DESKTOP_H

#include "types.h"

enum { APP_TERMINAL = 0, APP_FILES, APP_NOTES, APP_CALC, APP_PAINT, APP_SETTINGS };

void desktop_start(uint32_t mb_info_addr); /* enter graphics (bootloader FB / VBE) + draw chrome */
void desktop_draw(void);         /* (re)draw the desktop chrome */
void desktop_run(void);          /* mouse + dock + apps event loop (never returns) */
void desktop_open_app(int app);  /* switch the active app */

/* Theme (set from Settings > Appearance): wallpaper gradient + accent colour. */
void     desktop_set_theme(uint32_t wall_top, uint32_t wall_bot, uint32_t accent);
uint32_t desktop_accent(void);

#endif /* THUOS_DESKTOP_H */
