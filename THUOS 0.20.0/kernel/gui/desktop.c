/* THUOS — the THU Desktop: window manager + event loop (high-res).
 * Flat dark theme: gradient wallpaper, a top bar with a clock, a shadowed
 * rounded window, and a dock of pictogram app icons. Runs a mouse cursor and
 * dispatches keyboard/mouse to the active app (Terminal/Files/Notes/Calculator/
 * Paint/Settings). Settings > Appearance can change the theme at runtime. */
#include "desktop.h"
#include "lfb.h"
#include "gconsole.h"
#include "version.h"
#include "mouse.h"
#include "keyboard.h"
#include "shell.h"
#include "apps.h"
#include "kprintf.h"
#include "xhci.h"
#include "pit.h"

#define NAPP 6

static int W, H;
static int wx, wy, ww, wh, th;          /* window */
static int px, py, pw, ph;              /* content panel */
static int gcx, gcy, gcols, grows;      /* terminal console region */
static int dbx, dby, dbw, dbh;          /* dock bar */
static int ds = 52, dgap = 18, dn = NAPP;
static int g_app = APP_TERMINAL;

/* theme (Settings > Appearance) */
static uint32_t g_wall_top = 0x0a1020, g_wall_bot = 0x18243f, g_accent = 0x6cc7ff;

static const char *APPNAME[NAPP] = { "THU Terminal", "Files", "Notes", "Calculator", "Paint", "Settings" };
static const uint32_t TILE[NAPP] = { 0x2d3650, 0x3d8bdc, 0x2bb5a0, 0xff9f1c, 0xff6f91, 0x7c8aa0 };

/* ---- mouse cursor (composited) ---- */
#define CUR_W 12
#define CUR_H 19
static const char *arrow[CUR_H] = {
    "o...........", "oo..........", "oXo.........", "oXXo........",
    "oXXXo.......", "oXXXXo......", "oXXXXXo.....", "oXXXXXXo....",
    "oXXXXXXXo...", "oXXXXXXXXo..", "oXXXXXXXXXo.", "oXXXXXoooooo",
    "oXXoXXo.....", "oXo.oXXo....", "oo..oXXo....", "o....oXXo...",
    ".....oXXo...", "......oo....", "............",
};
static uint32_t cur_save[CUR_W * CUR_H];
static int cur_x, cur_y, cur_vis = 0;

static void cursor_hide(void) {
    if (cur_vis) { lfb_blit_put(cur_x, cur_y, CUR_W, CUR_H, cur_save); cur_vis = 0; }
}
static void cursor_show(int x, int y) {
    if (cur_vis) cursor_hide();
    lfb_blit_get(x, y, CUR_W, CUR_H, cur_save);
    for (int r = 0; r < CUR_H; r++)
        for (int c = 0; c < CUR_W && arrow[r][c]; c++) {
            char p = arrow[r][c];
            if (p == 'X')      lfb_pixel(x + c, y + r, 0xffffff);
            else if (p == 'o') lfb_pixel(x + c, y + r, 0x0a0f1a);
        }
    cur_x = x; cur_y = y; cur_vis = 1;
}

/* ---- pictogram app icons (drawn inside a tile) ---- */
static void icon_glyph(int app, int bx, int by, int s) {
    uint32_t WH = 0xffffff;
    switch (app) {
        case APP_TERMINAL: {
            lfb_round_fill(bx, by, s, s, 4, 0x0a1020);
            uint32_t g = 0x5be58a; int my = by + s / 2;
            lfb_line(bx + 6, by + 7, bx + 6 + 8, my, g);
            lfb_line(bx + 6, by + 8, bx + 6 + 8, my + 1, g);
            lfb_line(bx + 6, by + s - 7, bx + 6 + 8, my, g);
            lfb_line(bx + 6, by + s - 8, bx + 6 + 8, my - 1, g);
            lfb_fill(bx + s / 2, by + s - 10, 10, 2, g);
            break;
        }
        case APP_FILES:
            lfb_round_fill(bx, by + 5, s * 5 / 9, 6, 2, 0xdbe7f7);
            lfb_round_fill(bx, by + 8, s, s - 11, 3, WH);
            lfb_round_fill(bx, by + s / 2, s, s / 2 - 3, 3, 0xeaf2ff);
            break;
        case APP_NOTES: {
            lfb_round_fill(bx + 2, by, s - 4, s, 3, WH);          /* page */
            for (int i = 0; i < 4; i++)
                lfb_fill(bx + 7, by + 7 + i * 6, s - 14 - (i == 3 ? 8 : 0), 2, 0x9fb3d8);
            break;
        }
        case APP_CALC: {
            lfb_round_fill(bx, by, s, s, 4, WH);
            lfb_fill(bx + 4, by + 4, s - 8, 8, 0x2d3650);
            int g = 3, n = 3, bs = (s - 8 - (n - 1) * g) / n;
            for (int r = 0; r < n; r++)
                for (int c = 0; c < n; c++)
                    lfb_round_fill(bx + 4 + c * (bs + g), by + 16 + r * (bs + g), bs, bs, 1,
                                   (r == n - 1 && c == n - 1) ? 0xff9f1c : 0x2d3650);
            break;
        }
        case APP_PAINT: {
            lfb_round_fill(bx, by, s, s, s / 2, WH);              /* palette blob */
            lfb_disc(bx + s / 3,     by + s / 3,     3, 0xff5f56);
            lfb_disc(bx + 2 * s / 3, by + s / 3,     3, 0xffbd2e);
            lfb_disc(bx + s / 3,     by + 2 * s / 3, 3, 0x27c93f);
            lfb_disc(bx + 2 * s / 3, by + 2 * s / 3, 3, 0x6cc7ff);
            break;
        }
        case APP_SETTINGS: {                                     /* sliders */
            int kx[3] = { bx + s - 9, bx + 8, bx + s / 2 };
            for (int i = 0; i < 3; i++) {
                int yy = by + 5 + i * ((s - 8) / 2);
                lfb_round_fill(bx, yy, s, 4, 2, 0xe7ecf5);
                lfb_disc(kx[i], yy + 2, 4, WH);
                lfb_disc(kx[i], yy + 2, 2, 0x7c8aa0);
            }
            break;
        }
    }
}

static void draw_dock(void) {
    lfb_round_fill(dbx + 4, dby + 6, dbw, dbh, 20, 0x05080f);
    lfb_round_fill(dbx, dby, dbw, dbh, 20, 0x161f33);
    for (int i = 0; i < dn; i++) {
        int ax = dbx + 14 + i * (ds + dgap), ay = dby + 10, pad = 11;
        lfb_round_fill(ax, ay, ds, ds, 13, TILE[i]);
        icon_glyph(i, ax + pad, ay + pad, ds - 2 * pad);
        if (i == g_app) lfb_disc(ax + ds / 2, dby + dbh - 5, 2, g_accent);
    }
}

static void draw_clock(void) {
    uint32_t t = pit_seconds(), mm = (t / 60) % 100, ss = t % 60;
    char b[6];
    b[0] = '0' + mm / 10; b[1] = '0' + mm % 10; b[2] = ':';
    b[3] = '0' + ss / 10; b[4] = '0' + ss % 10; b[5] = 0;
    int tw = 5 * 16;
    lfb_fill(W - tw - 18, 3, tw + 14, 30, 0x0e1626);
    lfb_text(W - tw - 14, 6, b, 0xaec2e6, -1, 2);
}

/* ---- chrome ---- */
void desktop_draw(void) {
    W = lfb_width(); H = lfb_height();
    lfb_gradient_v(0, 0, W, H, g_wall_top, g_wall_bot);

    int tb = 38;
    lfb_fill(0, 0, W, tb, 0x0e1626);
    lfb_fill(0, tb, W, 2, 0x223456);
    lfb_round_fill(16, 11, 16, 16, 4, g_accent);
    lfb_text(42, 6, THUOS_NAME " " THUOS_VERSION, 0xe8eefc, -1, 2);
    draw_clock();

    ww = 820; wh = 552; wx = (W - ww) / 2; wy = 84; th = 40;
    lfb_round_fill(wx + 10, wy + 12, ww, wh, 18, 0x05080f);
    lfb_round_fill(wx, wy, ww, wh, 18, 0x0f1626);
    lfb_round_fill(wx, wy, ww, th, 18, 0x1d2b46);
    lfb_fill(wx, wy + th - 18, ww, 18, 0x1d2b46);
    lfb_fill(wx + 1, wy + th, ww - 2, 1, 0x2a3b5e);
    lfb_fill(wx + 20, wy + 15, 12, 12, 0xff5f56);
    lfb_fill(wx + 44, wy + 15, 12, 12, 0xffbd2e);
    lfb_fill(wx + 68, wy + 15, 12, 12, 0x27c93f);

    int pad = 16;
    px = wx + pad; py = wy + th + pad; pw = ww - 2 * pad; ph = wh - th - 2 * pad;
    gcx = px + 10; gcy = py + 8; gcols = (pw - 20) / 16; grows = (ph - 16) / 32;

    dbw = dn * ds + (dn - 1) * dgap + 28; dbh = ds + 20;
    dbx = (W - dbw) / 2; dby = H - dbh - 14;
    draw_dock();
}

static void titlebar_name(int app) {
    lfb_fill(wx + 96, wy + 2, 280, 36, 0x1d2b46);
    lfb_text(wx + 104, wy + 5, APPNAME[app], 0xe8eefc, -1, 2);
}

void desktop_open_app(int app) {
    g_app = app;
    titlebar_name(app);
    if (app == APP_TERMINAL) {
        lfb_round_fill(px, py, pw, ph, 8, 0x0a0f1a);
        gcon_init(gcx, gcy, gcols, grows, 2, 0x9defb0, 0x0a0f1a);
        kprintf("Welcome to %s %s \"%s\".\n", THUOS_NAME, THUOS_VERSION, THUOS_CODENAME);
        shell_start();
    } else {
        gcon_set_active(0);
        switch (app) {
            case APP_FILES:    app_files_open(px, py, pw, ph);    break;
            case APP_NOTES:    app_notes_open(px, py, pw, ph);    break;
            case APP_CALC:     app_calc_open(px, py, pw, ph);     break;
            case APP_PAINT:    app_paint_open(px, py, pw, ph);    break;
            case APP_SETTINGS: app_settings_open(px, py, pw, ph); break;
        }
    }
    draw_dock();
}

void desktop_set_theme(uint32_t top, uint32_t bot, uint32_t accent) {
    g_wall_top = top; g_wall_bot = bot; g_accent = accent;
    desktop_draw();
    desktop_open_app(g_app);     /* keep the current app on screen */
}
uint32_t desktop_accent(void) { return g_accent; }

static int dock_hit(int mx, int my) {
    for (int i = 0; i < dn; i++) {
        int ax = dbx + 14 + i * (ds + dgap), ay = dby + 10;
        if (mx >= ax && mx < ax + ds && my >= ay && my < ay + ds) return i;
    }
    return -1;
}

static void route_key(char c) {
    if (c == 27) { if (g_app != APP_TERMINAL) desktop_open_app(APP_TERMINAL); return; }
    if (g_app == APP_TERMINAL) shell_feed_char(c);
    else if (g_app == APP_CALC)  app_calc_key(c);
    else if (g_app == APP_NOTES) app_notes_key(c);
}

static int in_panel(int mx, int my) { return mx >= px && mx < px + pw && my >= py && my < py + ph; }

static void handle_click(int mx, int my) {
    int a = dock_hit(mx, my);
    if (a >= 0) { desktop_open_app(a); return; }
    if (!in_panel(mx, my)) return;
    if (g_app == APP_CALC)          app_calc_click(mx, my);
    else if (g_app == APP_FILES)    app_files_click(mx, my);
    else if (g_app == APP_PAINT)    app_paint_click(mx, my);
    else if (g_app == APP_SETTINGS) app_settings_click(mx, my);
}

void desktop_run(void) {
    W = lfb_width(); H = lfb_height();
    mouse_init(W, H);
    desktop_open_app(APP_TERMINAL);     /* prints the prompt -> serial (boot self-test) */
    cursor_show(mouse_x(), mouse_y());

    int prevl = 0;
    uint32_t last_sec = 0xffffffffu;
    for (;;) {
        __asm__ volatile("hlt");
        usb_poll();                 /* deliver USB HID keyboard/mouse reports */
        int evt = 0;
        uint32_t s = pit_seconds();
        if (s != last_sec) { last_sec = s; if (!evt) { cursor_hide(); evt = 1; } draw_clock(); }
        while (keyboard_haskey()) {
            if (!evt) { cursor_hide(); evt = 1; }
            route_key(keyboard_trygetchar());
        }
        if (mouse_take_event()) {
            if (!evt) { cursor_hide(); evt = 1; }
            int l = mouse_left();
            if (l && !prevl) handle_click(mouse_x(), mouse_y());
            else if (l && g_app == APP_PAINT && in_panel(mouse_x(), mouse_y()))
                app_paint_motion(mouse_x(), mouse_y());     /* drag to draw */
            prevl = l;
        }
        if (evt) cursor_show(mouse_x(), mouse_y());
    }
}

void desktop_start(uint32_t mb_info_addr) {
    if (!lfb_init_auto(mb_info_addr)) return;   /* bootloader FB on real HW, else VBE */
    desktop_draw();
}
