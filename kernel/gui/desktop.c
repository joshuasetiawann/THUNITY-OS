/* THUOS — the THU Desktop: window manager + event loop (high-res).
 * Draws a flat dark desktop (wallpaper, top bar, a window, a dock), runs a
 * mouse cursor, and dispatches keyboard/mouse to the active app. The terminal
 * (the shell) is one app among Calculator, Files, System and About. */
#include "desktop.h"
#include "lfb.h"
#include "gconsole.h"
#include "version.h"
#include "mouse.h"
#include "keyboard.h"
#include "shell.h"
#include "apps.h"
#include "kprintf.h"

static int W, H;
static int wx, wy, ww, wh, th;          /* window */
static int px, py, pw, ph;              /* content panel (apps draw here) */
static int gcx, gcy, gcols, grows;      /* terminal console region */
static int dn = 5, ds = 44, dgap = 14, dx0, dy0, dh = 60;
static int g_app = APP_TERMINAL;

static const char     dock_glyph[5] = { 'T', '=', 'F', 'S', 'i' };
static const uint32_t dock_col[5]   = { 0x6cc7ff, 0x27c93f, 0xffbd2e, 0xff7b9c, 0xb18cff };
static const char    *APPNAME[5]    = { "THU Terminal", "Calculator", "Files", "System", "About" };

static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }

/* ---- mouse cursor (composited: save background, draw arrow, restore) ---- */
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

/* ---- chrome ---- */
void desktop_draw(void) {
    W = lfb_width(); H = lfb_height();
    lfb_gradient_v(0, 0, W, H, 0x0b1220, 0x16223a);

    int tb = 34;
    lfb_fill(0, 0, W, tb, 0x101a2e);
    lfb_fill(0, tb, W, 2, 0x263a5e);
    lfb_round_fill(14, 9, 16, 16, 4, 0x6cc7ff);
    lfb_text(40, 9, THUOS_NAME " " THUOS_VERSION, 0xe8eefc, -1, 2);
    const char *cn = THUOS_CODENAME;
    lfb_text(W - slen(cn) * 16 - 16, 9, cn, 0x8aa0c0, -1, 2);

    ww = 820; wh = 560; wx = (W - ww) / 2; wy = 78; th = 40;
    lfb_round_fill(wx + 8, wy + 10, ww, wh, 16, 0x05080f);
    lfb_round_fill(wx, wy, ww, wh, 16, 0x0f1626);
    lfb_round_fill(wx, wy, ww, th, 16, 0x1b2740);
    lfb_fill(wx, wy + th - 16, ww, 16, 0x1b2740);
    lfb_fill(wx + 20, wy + 15, 12, 12, 0xff5f56);
    lfb_fill(wx + 44, wy + 15, 12, 12, 0xffbd2e);
    lfb_fill(wx + 68, wy + 15, 12, 12, 0x27c93f);

    /* content panel + terminal console region */
    int pad = 16;
    px = wx + pad; py = wy + th + pad; pw = ww - 2 * pad; ph = wh - th - 2 * pad;
    gcx = px + 10; gcy = py + 8; gcols = (pw - 20) / 16; grows = (ph - 16) / 32;

    /* dock */
    int dw = dn * ds + (dn - 1) * dgap + 24;
    dx0 = (W - dw) / 2; dy0 = H - dh - 14;
    lfb_round_fill(dx0 + 4, dy0 + 5, dw, dh, 18, 0x05080f);
    lfb_round_fill(dx0, dy0, dw, dh, 18, 0x18223a);
    for (int i = 0; i < dn; i++) {
        int ax = dx0 + 12 + i * (ds + dgap), ay = dy0 + (dh - ds) / 2;
        lfb_round_fill(ax, ay, ds, ds, 12, dock_col[i]);
        char g[2] = { dock_glyph[i], 0 };
        lfb_char(ax + (ds - 16) / 2, ay + (ds - 32) / 2, g[0], 0x0f1626, -1, 2);
    }
}

static void titlebar_name(int app) {
    lfb_fill(wx + 96, wy + 2, 260, 36, 0x1b2740);   /* clear (covers scale-2 height) */
    lfb_text(wx + 104, wy + 5, APPNAME[app], 0xe8eefc, -1, 2);
}

void desktop_open_app(int app) {
    g_app = app;
    titlebar_name(app);
    switch (app) {
        case APP_TERMINAL:
            lfb_round_fill(px, py, pw, ph, 8, 0x0a0f1a);
            gcon_init(gcx, gcy, gcols, grows, 2, 0x9defb0, 0x0a0f1a);
            kprintf("Welcome to %s %s \"%s\".\n", THUOS_NAME, THUOS_VERSION, THUOS_CODENAME);
            shell_start();
            break;
        case APP_CALC:   gcon_set_active(0); app_calc_open(px, py, pw, ph);  break;
        case APP_FILES:  gcon_set_active(0); app_files_open(px, py, pw, ph); break;
        case APP_SYSTEM: gcon_set_active(0); app_sys_open(px, py, pw, ph);   break;
        case APP_ABOUT:  gcon_set_active(0); app_about_open(px, py, pw, ph);  break;
    }
}

static int dock_hit(int mx, int my) {
    for (int i = 0; i < dn; i++) {
        int ax = dx0 + 12 + i * (ds + dgap), ay = dy0 + (dh - ds) / 2;
        if (mx >= ax && mx < ax + ds && my >= ay && my < ay + ds) return i;
    }
    return -1;
}

static void route_key(char c) {
    if (c == 27) { if (g_app != APP_TERMINAL) desktop_open_app(APP_TERMINAL); return; } /* Esc */
    if (g_app == APP_TERMINAL) shell_feed_char(c);
    else if (g_app == APP_CALC) app_calc_key(c);
}

static void handle_click(int mx, int my) {
    int a = dock_hit(mx, my);
    if (a >= 0) { desktop_open_app(a); return; }
    if (mx >= px && mx < px + pw && my >= py && my < py + ph) {
        if (g_app == APP_CALC)  app_calc_click(mx, my);
        else if (g_app == APP_FILES) app_files_click(mx, my);
    }
}

void desktop_run(void) {
    W = lfb_width(); H = lfb_height();
    mouse_init(W, H);
    desktop_open_app(APP_TERMINAL);     /* prints the prompt -> serial (boot self-test) */
    cursor_show(mouse_x(), mouse_y());

    int prevl = 0;
    for (;;) {
        __asm__ volatile("hlt");        /* idle until the next IRQ */
        int evt = 0;
        while (keyboard_haskey()) {
            if (!evt) { cursor_hide(); evt = 1; }
            route_key(keyboard_trygetchar());
        }
        if (mouse_take_event()) {
            if (!evt) { cursor_hide(); evt = 1; }
            int l = mouse_left();
            if (l && !prevl) handle_click(mouse_x(), mouse_y());
            prevl = l;
        }
        if (evt) cursor_show(mouse_x(), mouse_y());
    }
}

void desktop_start(void) {
    if (!lfb_init(1024, 768)) return;   /* no framebuffer -> caller falls back to text */
    desktop_draw();
}
