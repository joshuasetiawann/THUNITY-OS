/* THUOS — the THU Desktop (modern, high-res). See desktop.h.
 * A flat dark theme drawn into the 1024x768 truecolour framebuffer: gradient
 * wallpaper, a top bar, a shadowed rounded window hosting the terminal, and a
 * dock. The interactive shell runs inside the terminal panel. */
#include "desktop.h"
#include "lfb.h"
#include "gconsole.h"
#include "version.h"

static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }

void desktop_draw(void) {
    int W = lfb_width(), H = lfb_height();

    lfb_gradient_v(0, 0, W, H, 0x0b1220, 0x16223a);          /* wallpaper */

    /* top bar */
    int tb = 34;
    lfb_fill(0, 0, W, tb, 0x101a2e);
    lfb_fill(0, tb, W, 2, 0x263a5e);                         /* divider */
    lfb_round_fill(14, 9, 16, 16, 4, 0x6cc7ff);              /* brand dot */
    lfb_text(40, 9, THUOS_NAME " " THUOS_VERSION, 0xe8eefc, -1, 2);
    const char *cn = THUOS_CODENAME;
    lfb_text(W - slen(cn) * 16 - 16, 9, cn, 0x8aa0c0, -1, 2);

    /* window */
    int ww = 820, wh = 560, wx = (W - ww) / 2, wy = 78;
    lfb_round_fill(wx + 8, wy + 10, ww, wh, 16, 0x05080f);   /* drop shadow */
    lfb_round_fill(wx, wy, ww, wh, 16, 0x0f1626);            /* body */
    int th = 40;                                             /* title bar */
    lfb_round_fill(wx, wy, ww, th, 16, 0x1b2740);
    lfb_fill(wx, wy + th - 16, ww, 16, 0x1b2740);            /* square the bottom */
    lfb_fill(wx + 20, wy + 15, 12, 12, 0xff5f56);            /* traffic lights */
    lfb_fill(wx + 44, wy + 15, 12, 12, 0xffbd2e);
    lfb_fill(wx + 68, wy + 15, 12, 12, 0x27c93f);
    lfb_text(wx + 104, wy + 10, "THU Terminal", 0xe8eefc, -1, 2);

    /* terminal panel -> graphical console */
    int pad = 16;
    int tx = wx + pad, ty = wy + th + pad, tw = ww - 2 * pad, tcyh = wh - th - 2 * pad;
    lfb_round_fill(tx, ty, tw, tcyh, 8, 0x0a0f1a);
    int s = 2, cw = 8 * s, chh = 16 * s;
    int cols = (tw - 20) / cw, rows = (tcyh - 16) / chh;
    gcon_init(tx + 10, ty + 8, cols, rows, s, 0x9defb0, 0x0a0f1a);

    /* dock */
    int dn = 5, ds = 44, dgap = 14, dw = dn * ds + (dn - 1) * dgap + 24, dh = 60;
    int dx = (W - dw) / 2, dy = H - dh - 14;
    lfb_round_fill(dx + 4, dy + 5, dw, dh, 18, 0x05080f);    /* dock shadow */
    lfb_round_fill(dx, dy, dw, dh, 18, 0x18223a);
    static const char     gl[5] = { 'T', 'F', 'S', 'i', '+' };
    static const uint32_t ic[5] = { 0x6cc7ff, 0x27c93f, 0xffbd2e, 0xff7b9c, 0xb18cff };
    for (int i = 0; i < dn; i++) {
        int ax = dx + 12 + i * (ds + dgap), ay = dy + (dh - ds) / 2;
        lfb_round_fill(ax, ay, ds, ds, 12, ic[i]);
        lfb_char(ax + (ds - 16) / 2, ay + (ds - 32) / 2, gl[i], 0x0f1626, -1, 2);
    }
}

void desktop_start(void) {
    if (!lfb_init(1024, 768)) return;   /* no framebuffer -> stay on the text console */
    desktop_draw();
}
