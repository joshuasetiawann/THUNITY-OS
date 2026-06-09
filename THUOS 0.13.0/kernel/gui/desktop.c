/* THUOS — the THU Desktop. See desktop.h. */
#include "desktop.h"
#include "gfx.h"
#include "gconsole.h"
#include "version.h"

static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }

void desktop_draw(void) {
    gfx_clear(COL_DESK1);
    gfx_fill(0, 0, GFX_W, 100, COL_DESK2);             /* subtle two-tone sky */

    /* top bar */
    gfx_fill(0, 0, GFX_W, 16, COL_TOPBAR);
    gfx_fill(4, 4, 8, 8, COL_BRAND);                   /* brand dot */
    gfx_text(15, 0, THUOS_NAME " " THUOS_VERSION, COL_TITLETX, COL_TOPBAR);
    const char *cn = THUOS_CODENAME;
    gfx_text(GFX_W - slen(cn) * 8 - 4, 0, cn, COL_ACCENT, COL_TOPBAR);

    /* taskbar */
    gfx_fill(0, GFX_H - 16, GFX_W, 16, COL_TASKBAR);
    gfx_text(4, GFX_H - 16, "THU  -  thuos terminal", COL_MUTED, COL_TASKBAR);
    gfx_fill(GFX_W - 12, GFX_H - 12, 8, 8, COL_LGREEN);

    /* window */
    int wx = 4, wy = 18, ww = 312, wh = 164;
    gfx_fill(wx + 3, wy + 3, ww, wh, COL_SHADOW);      /* drop shadow */
    gfx_fill(wx, wy, ww, wh, COL_WIN);
    gfx_rect(wx, wy, ww, wh, COL_WINBORDER);
    gfx_fill(wx + 1, wy + 1, ww - 2, 15, COL_TITLE);   /* title bar */
    gfx_fill(wx + 7,  wy + 5, 6, 6, COL_LRED);         /* window controls */
    gfx_fill(wx + 16, wy + 5, 6, 6, COL_YELLOW);
    gfx_fill(wx + 25, wy + 5, 6, 6, COL_LGREEN);
    gfx_text(wx + 38, wy, "THU Terminal", COL_TITLETX, COL_TITLE);

    /* terminal content area -> hand it to the graphical console */
    int tx = wx + 3, ty = wy + 17, tw = ww - 6, th = wh - 20;
    gfx_fill(tx, ty, tw, th, COL_TERMBG);
    int cols = tw / 8, rows = th / 16;
    gcon_init(tx + (tw - cols * 8) / 2, ty + (th - rows * 16) / 2, cols, rows);
}

void desktop_start(void) {
    gfx_enter();        /* VGA mode 13h + palette */
    desktop_draw();     /* chrome + console */
}
