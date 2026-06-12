/* THUOS — built-in desktop apps. Each app draws into the window content area
 * given at open(); the desktop routes keyboard/mouse to the active app. */
#ifndef THUOS_APPS_H
#define THUOS_APPS_H

void app_calc_open(int x, int y, int w, int h);
void app_calc_key(char c);
void app_calc_click(int mx, int my);

void app_files_open(int x, int y, int w, int h);
void app_files_click(int mx, int my);

void app_notes_open(int x, int y, int w, int h);
void app_notes_key(char c);

void app_paint_open(int x, int y, int w, int h);
void app_paint_click(int mx, int my);
void app_paint_motion(int mx, int my);     /* called while the button is held */

void app_settings_open(int x, int y, int w, int h);   /* sidebar menu + panes */
void app_settings_click(int mx, int my);

#endif /* THUOS_APPS_H */
