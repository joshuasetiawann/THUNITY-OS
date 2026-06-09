/* THUOS — built-in desktop apps. Each app draws into the window content area
 * given at open(); the desktop routes keyboard/mouse to the active app. */
#ifndef THUOS_APPS_H
#define THUOS_APPS_H

void app_calc_open(int x, int y, int w, int h);
void app_calc_key(char c);
void app_calc_click(int mx, int my);

void app_files_open(int x, int y, int w, int h);
void app_files_click(int mx, int my);

void app_sys_open(int x, int y, int w, int h);     /* honest system + devices panel */
void app_about_open(int x, int y, int w, int h);

#endif /* THUOS_APPS_H */
