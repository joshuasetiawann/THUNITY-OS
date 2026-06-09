/* THUOS — built-in desktop apps: Calculator, Files, Notes, Paint, Settings.
 * Drawn into the window content rectangle with the lfb primitives. */
#include "apps.h"
#include "desktop.h"
#include "lfb.h"
#include "version.h"
#include "fs.h"
#include "pit.h"
#include "pmm.h"
#include "kheap.h"

/* ---- helpers ---- */
static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }

static char *itoa_l(long v, char *buf) {
    char tmp[24]; int i = 0, neg = 0;
    unsigned long u;
    if (v < 0) { neg = 1; u = (unsigned long)(-(v + 1)) + 1; } else u = (unsigned long)v;
    if (u == 0) tmp[i++] = '0';
    while (u) { tmp[i++] = (char)('0' + u % 10); u /= 10; }
    int j = 0;
    if (neg) buf[j++] = '-';
    while (i) buf[j++] = tmp[--i];
    buf[j] = '\0';
    return buf;
}

static int text_block(int x, int y, int w, const char *s, uint32_t fg, int scale) {
    int cw = 8 * scale, lh = 16 * scale, cols = w / cw, col = 0, cx = x;
    for (; *s; s++) {
        if (*s == '\n') { y += lh; cx = x; col = 0; continue; }
        if (col >= cols) { y += lh; cx = x; col = 0; }
        lfb_char(cx, y, *s, fg, -1, scale);
        cx += cw; col++;
    }
    return y + lh;
}

/* theme tokens */
#define PANEL_BG 0x0a0f1a
#define CARD     0x121a2b
#define CARD2    0x1b263d
#define TXT_HI   0xe8eefc
#define TXT_LO   0x8aa0c0
#define ACCENT   0x6cc7ff
#define GOOD     0x47d18a
#define WARN     0xffbd2e
#define BAD      0xff6b6b

/* ======================= Calculator ======================= */
static int   cc_x, cc_y, cc_w, cc_h, cc_bx, cc_by, cc_cw, cc_ch, cc_gap;
static long  cc_acc, cc_cur;
static char  cc_op;
static int   cc_fresh;
static long  cc_disp;
static const char *CALC_LBL[4][4] = {
    { "7", "8", "9", "/" }, { "4", "5", "6", "*" },
    { "1", "2", "3", "-" }, { "0", "=", "+", "C" },
};
static void calc_draw_display(void) {
    lfb_round_fill(cc_x + 14, cc_y + 14, cc_w - 28, 56, 8, 0x05080f);
    char buf[24]; itoa_l(cc_disp, buf);
    lfb_text(cc_x + cc_w - 28 - slen(buf) * 16, cc_y + 30, buf, GOOD, -1, 2);
}
static void calc_draw_buttons(void) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            int bx = cc_bx + c * (cc_cw + cc_gap), by = cc_by + r * (cc_ch + cc_gap);
            uint32_t bg = (c == 3) ? 0x2a3a5e : CARD2;
            if (r == 3 && c == 1) bg = ACCENT;
            if (r == 3 && c == 3) bg = 0x5a2a3a;
            lfb_round_fill(bx, by, cc_cw, cc_ch, 8, bg);
            uint32_t fg = (r == 3 && c == 1) ? 0x06121f : TXT_HI;
            lfb_text(bx + (cc_cw - 16) / 2, by + (cc_ch - 32) / 2, CALC_LBL[r][c], fg, -1, 2);
        }
}
void app_calc_open(int x, int y, int w, int h) {
    lfb_fill(x, y, w, h, PANEL_BG);
    cc_w = 300; cc_h = 400;
    cc_x = x + (w - cc_w) / 2; cc_y = y + (h - cc_h) / 2; if (cc_y < y + 12) cc_y = y + 12;
    lfb_round_fill(cc_x, cc_y, cc_w, cc_h, 14, CARD);
    lfb_text(cc_x + 14, cc_y - 26, "Calculator", TXT_LO, -1, 1);
    cc_gap = 10; cc_bx = cc_x + 14; cc_by = cc_y + 84;
    cc_cw = (cc_w - 28 - 3 * cc_gap) / 4; cc_ch = (cc_h - 84 - 14 - 3 * cc_gap) / 4;
    cc_acc = cc_cur = 0; cc_op = 0; cc_fresh = 1; cc_disp = 0;
    calc_draw_display(); calc_draw_buttons();
}
static long calc_apply(long a, char op, long b) {
    switch (op) { case '+': return a + b; case '-': return a - b;
                  case '*': return a * b; case '/': return b ? a / b : 0; }
    return b;
}
void app_calc_key(char c) {
    if (c >= '0' && c <= '9') {
        if (cc_fresh) { cc_cur = 0; cc_fresh = 0; }
        cc_cur = cc_cur * 10 + (c - '0'); cc_disp = cc_cur;
    } else if (c == '+' || c == '-' || c == '*' || c == '/') {
        cc_acc = cc_op ? calc_apply(cc_acc, cc_op, cc_cur) : cc_cur;
        cc_op = c; cc_fresh = 1; cc_disp = cc_acc;
    } else if (c == '=' || c == '\n') {
        cc_acc = cc_op ? calc_apply(cc_acc, cc_op, cc_cur) : cc_cur;
        cc_op = 0; cc_disp = cc_acc; cc_cur = cc_acc; cc_fresh = 1;
    } else if (c == 'c' || c == 'C') {
        cc_acc = cc_cur = 0; cc_op = 0; cc_fresh = 1; cc_disp = 0;
    } else return;
    calc_draw_display();
}
void app_calc_click(int mx, int my) {
    for (int r = 0; r < 4; r++)
        for (int col = 0; col < 4; col++) {
            int bx = cc_bx + col * (cc_cw + cc_gap), by = cc_by + r * (cc_ch + cc_gap);
            if (mx >= bx && mx < bx + cc_cw && my >= by && my < by + cc_ch) {
                app_calc_key(CALC_LBL[r][col][0]); calc_draw_buttons(); return;
            }
        }
}

/* ======================= Files ======================= */
static int fa_x, fa_y, fa_w, fa_h, fa_rowy, fa_rowh, fa_listx, fa_listw;
void app_files_open(int x, int y, int w, int h) {
    fa_x = x; fa_y = y; fa_w = w; fa_h = h;
    lfb_fill(x, y, w, h, PANEL_BG);
    lfb_text(x + 16, y + 12, "Files", TXT_HI, -1, 2);
    char buf[24];
    lfb_text(x + 16, y + 44, "ramfs - click a file to view", TXT_LO, -1, 1);
    lfb_text(x + 16, y + 62, itoa_l((long)kfs_bytes_used(), buf), TXT_LO, -1, 1);
    lfb_text(x + 16 + slen(buf) * 8 + 8, y + 62, "bytes used", TXT_LO, -1, 1);
    fa_listx = x + 16; fa_listw = (w - 32) / 2; fa_rowy = y + 90; fa_rowh = 28;
    int max = kfs_max_files(), shown = 0;
    for (int i = 0; i < max; i++) {
        const char *nm = kfs_name(i); if (!nm) continue;
        int ry = fa_rowy + shown * fa_rowh;
        lfb_round_fill(fa_listx, ry, fa_listw, fa_rowh - 6, 6, CARD2);
        lfb_text(fa_listx + 10, ry + 3, nm, TXT_HI, -1, 1); shown++;
    }
    if (!shown) lfb_text(fa_listx, fa_rowy, "(no files)", TXT_LO, -1, 1);
    lfb_round_fill(x + 16 + fa_listw + 12, y + 90, w - 32 - fa_listw - 12, h - 100, 8, 0x05080f);
    lfb_text(x + 16 + fa_listw + 24, y + 100, "select a file ->", TXT_LO, -1, 1);
}
void app_files_click(int mx, int my) {
    if (mx < fa_listx || mx > fa_listx + fa_listw) return;
    int row = (my - fa_rowy) / fa_rowh; if (row < 0) return;
    int max = kfs_max_files(), shown = 0;
    for (int i = 0; i < max; i++) {
        const char *nm = kfs_name(i); if (!nm) continue;
        if (shown == row) {
            int vx = fa_x + 16 + fa_listw + 12, vy = fa_y + 90;
            int vw = fa_w - 32 - fa_listw - 12, vh = fa_h - 100;
            lfb_round_fill(vx, vy, vw, vh, 8, 0x05080f);
            lfb_text(vx + 12, vy + 10, nm, ACCENT, -1, 1);
            uint32_t n; const uint8_t *d = kfs_read(nm, &n);
            if (d) {
                char tmp[512]; uint32_t k = 0;
                for (uint32_t j = 0; j < n && k < sizeof tmp - 1; j++) tmp[k++] = (char)d[j];
                tmp[k] = '\0'; text_block(vx + 12, vy + 34, vw - 24, tmp, TXT_HI, 1);
            }
            return;
        }
        shown++;
    }
}

/* ======================= Notes (editor, saved to ramfs) ======================= */
static int  no_tx, no_ty, no_tw, no_th;
static char no_buf[1024];
static int  no_len;
static void notes_render(void) {
    lfb_round_fill(no_tx, no_ty, no_tw, no_th, 8, 0x05080f);
    no_buf[no_len] = '\0';
    int endy = text_block(no_tx + 12, no_ty + 10, no_tw - 24, no_buf, TXT_HI, 2);
    (void)endy;
}
void app_notes_open(int x, int y, int w, int h) {
    lfb_fill(x, y, w, h, PANEL_BG);
    lfb_text(x + 16, y + 12, "Notes", TXT_HI, -1, 2);
    lfb_text(x + 16, y + 44, "type to edit - autosaved to notes.txt", TXT_LO, -1, 1);
    no_tx = x + 16; no_ty = y + 70; no_tw = w - 32; no_th = h - 86;
    uint32_t n; const uint8_t *d = kfs_read("notes.txt", &n);   /* load if present */
    no_len = 0;
    if (d) { for (uint32_t i = 0; i < n && no_len < (int)sizeof no_buf - 1; i++) no_buf[no_len++] = (char)d[i]; }
    notes_render();
}
void app_notes_key(char c) {
    if (c == '\b') { if (no_len > 0) no_len--; }
    else if (c == '\n' || c >= ' ') { if (no_len < (int)sizeof no_buf - 1) no_buf[no_len++] = c; }
    else return;
    kfs_write("notes.txt", no_buf, (uint32_t)no_len);          /* autosave */
    notes_render();
}

/* ======================= Paint (mouse drawing) ======================= */
static int pa_cx, pa_cy, pa_cw, pa_ch;          /* canvas */
static int pa_px, pa_py, pa_sw, pa_gap;         /* palette */
static uint32_t pa_col;
static int pa_clrx, pa_clry, pa_clrw, pa_clrh;  /* clear button */
static int pa_lx = -1, pa_ly = -1;              /* last stroke point */
static const uint32_t PA_PAL[7] =
    { 0xffffff, 0xff5f56, 0xffbd2e, 0x27c93f, 0x6cc7ff, 0xb18cff, 0x0a0f1a };
static int pa_in_canvas(int x, int y) {
    return x >= pa_cx + 4 && x < pa_cx + pa_cw - 4 && y >= pa_cy + 4 && y < pa_cy + pa_ch - 4;
}
void app_paint_open(int x, int y, int w, int h) {
    pa_lx = pa_ly = -1;
    lfb_fill(x, y, w, h, PANEL_BG);
    lfb_text(x + 16, y + 12, "Paint", TXT_HI, -1, 2);
    lfb_text(x + 16, y + 44, "drag on the canvas to draw - pick a colour below", TXT_LO, -1, 1);
    pa_cx = x + 16; pa_cy = y + 70; pa_cw = w - 32; pa_ch = h - 70 - 70;
    lfb_round_fill(pa_cx, pa_cy, pa_cw, pa_ch, 8, 0xf3f5fa);     /* white canvas */
    pa_sw = 40; pa_gap = 12; pa_px = x + 16; pa_py = pa_cy + pa_ch + 14;
    for (int i = 0; i < 7; i++) {
        int sx = pa_px + i * (pa_sw + pa_gap);
        lfb_round_fill(sx, pa_py, pa_sw, pa_sw, 8, PA_PAL[i]);
        if (i == 6) lfb_rect(sx, pa_py, pa_sw, pa_sw, 0x445);
    }
    pa_col = PA_PAL[6];
    pa_clrw = 90; pa_clrh = 40; pa_clrx = x + w - 16 - pa_clrw; pa_clry = pa_py;
    lfb_round_fill(pa_clrx, pa_clry, pa_clrw, pa_clrh, 8, CARD2);
    lfb_text(pa_clrx + 16, pa_clry + 4, "Clear", TXT_HI, -1, 2);
}
void app_paint_motion(int mx, int my) {
    if (!pa_in_canvas(mx, my)) { pa_lx = -1; return; }
    if (pa_lx < 0) { lfb_disc(mx, my, 3, pa_col); pa_lx = mx; pa_ly = my; return; }
    int dx = mx - pa_lx, dy = my - pa_ly;
    int steps = (dx < 0 ? -dx : dx), ady = (dy < 0 ? -dy : dy);
    if (ady > steps) steps = ady;
    if (steps < 1) steps = 1;
    for (int i = 0; i <= steps; i++)             /* interpolate -> solid stroke */
        lfb_disc(pa_lx + dx * i / steps, pa_ly + dy * i / steps, 3, pa_col);
    pa_lx = mx; pa_ly = my;
}
void app_paint_click(int mx, int my) {
    pa_lx = -1;
    for (int i = 0; i < 7; i++) {
        int sx = pa_px + i * (pa_sw + pa_gap);
        if (mx >= sx && mx < sx + pa_sw && my >= pa_py && my < pa_py + pa_sw) { pa_col = PA_PAL[i]; return; }
    }
    if (mx >= pa_clrx && mx < pa_clrx + pa_clrw && my >= pa_clry && my < pa_clry + pa_clrh) {
        lfb_round_fill(pa_cx, pa_cy, pa_cw, pa_ch, 8, 0xf3f5fa); return;
    }
    app_paint_motion(mx, my);
}

/* ======================= Settings (sidebar menu) ======================= */
static const char *SET_CAT[5] = { "Appearance", "Display", "Date & Time", "Devices", "About" };
static int set_cat = 0;
static int set_x, set_y, set_w, set_h, set_sw, set_rowh, set_cx, set_cy, set_cw, set_ch;
static const struct { const char *name; uint32_t top, bot, acc; } THEMES[5] = {
    { "Midnight", 0x0a1020, 0x18243f, 0x6cc7ff },
    { "Forest",   0x081a14, 0x12301f, 0x47d18a },
    { "Sunset",   0x1c0f18, 0x2e1622, 0xff7b54 },
    { "Grape",    0x140a24, 0x241640, 0xb18cff },
    { "Slate",    0x0e1116, 0x1a2230, 0x9fb3d8 },
};
static int set_sw_x[5], set_sw_y[5], set_sw_w = 150, set_sw_h = 64;  /* theme swatch rects */

static void settings_content(void) {
    lfb_fill(set_cx, set_cy, set_cw, set_ch, PANEL_BG);
    int x = set_cx + 18, y = set_cy + 14;
    lfb_text(x, y, SET_CAT[set_cat], TXT_HI, -1, 2); y += 44;
    char b[40];
    switch (set_cat) {
        case 0:  /* Appearance: theme swatches */
            lfb_text(x, y, "Theme - click to apply", TXT_LO, -1, 1); y += 26;
            for (int i = 0; i < 5; i++) {
                int sx = x + (i % 3) * (set_sw_w + 16), sy = y + (i / 3) * (set_sw_h + 16);
                set_sw_x[i] = sx; set_sw_y[i] = sy;
                lfb_round_fill(sx, sy, set_sw_w, set_sw_h, 8, THEMES[i].top);
                lfb_round_fill(sx, sy + set_sw_h / 2, set_sw_w, set_sw_h / 2, 8, THEMES[i].bot);
                lfb_disc(sx + 18, sy + set_sw_h / 2, 9, THEMES[i].acc);
                lfb_text(sx + 36, sy + set_sw_h / 2 - 8, THEMES[i].name, TXT_HI, -1, 1);
                if (THEMES[i].acc == desktop_accent()) lfb_rect(sx - 2, sy - 2, set_sw_w + 4, set_sw_h + 4, 0xffffff);
            }
            break;
        case 1:  /* Display */
            lfb_text(x, y, "Resolution : 1024 x 768", TXT_HI, -1, 1); y += 22;
            lfb_text(x, y, "Colour     : 32-bit truecolor", TXT_HI, -1, 1); y += 22;
            lfb_text(x, y, "Driver     : Bochs VBE linear framebuffer", TXT_HI, -1, 1); y += 22;
            lfb_text(x, y, "Backend    : QEMU std VGA (PCI BAR)", TXT_LO, -1, 1);
            break;
        case 2:  /* Date & Time */
            lfb_text(x, y, "Uptime : ", TXT_HI, -1, 1);
            lfb_text(x + 80, y, itoa_l((long)pit_seconds(), b), TXT_HI, -1, 1);
            lfb_text(x + 80 + slen(b) * 8 + 4, y, "seconds", TXT_HI, -1, 1); y += 22;
            lfb_text(x, y, "Tick   : PIT @ 100 Hz", TXT_HI, -1, 1); y += 22;
            lfb_text(x, y, "Clock  : mm:ss shown in the top bar", TXT_LO, -1, 1); y += 22;
            lfb_text(x, y, "(no battery-backed RTC wired yet)", TXT_LO, -1, 1);
            break;
        case 3: { /* Devices (honest) */
            struct { const char *n, *s; uint32_t c; } d[] = {
                { "Display ", "1024x768x32 (Bochs VBE)  OK", GOOD },
                { "Keyboard", "PS/2  OK", GOOD },
                { "Mouse   ", "PS/2  OK", GOOD },
                { "Storage ", "RAM filesystem (volatile)  OK", GOOD },
                { "Timer   ", "PIT 100 Hz  OK", GOOD },
                { "Network ", "emulated NIC may exist; no driver yet", WARN },
                { "Camera  ", "none (no device on this machine)", BAD },
                { "Wi-Fi   ", "not supported (needs vendor firmware)", BAD },
                { "Bluetooth", "not supported", BAD },
            };
            for (unsigned i = 0; i < sizeof d / sizeof d[0]; i++) {
                lfb_fill(x, y + 5, 8, 8, d[i].c);
                lfb_text(x + 16, y, d[i].n, TXT_HI, -1, 1);
                lfb_text(x + 16 + 9 * 8, y, d[i].s, TXT_LO, -1, 1); y += 20;
            }
            break;
        }
        case 4:  /* About */
            lfb_text(x, y, THUOS_NAME " " THUOS_VERSION " \"" THUOS_CODENAME "\"", ACCENT, -1, 1); y += 24;
            text_block(x, y, set_cw - 36,
                THUOS_ARCH "\n\n"
                "A from-scratch OS: a real x86 kernel in C + assembly,\n"
                "booting in QEMU and verified in CI over serial.\n\n"
                "Apps: Terminal, Files, Notes, Calculator, Paint,\n"
                "Settings. Click the dock; Esc returns to terminal.", TXT_LO, 1);
            break;
    }
}

static void settings_sidebar(void) {
    lfb_fill(set_x, set_y, set_sw, set_h, 0x0d1424);
    lfb_fill(set_x + set_sw, set_y, 1, set_h, 0x223456);
    lfb_text(set_x + 16, set_y + 14, "Settings", TXT_HI, -1, 2);
    for (int i = 0; i < 5; i++) {
        int ry = set_y + 56 + i * set_rowh;
        if (i == set_cat) lfb_round_fill(set_x + 8, ry, set_sw - 16, set_rowh - 6, 6, 0x1c2740);
        lfb_text(set_x + 18, ry + (set_rowh - 6 - 16) / 2, SET_CAT[i],
                 i == set_cat ? TXT_HI : TXT_LO, -1, 1);
    }
}

void app_settings_open(int x, int y, int w, int h) {
    set_x = x; set_y = y; set_w = w; set_h = h;
    set_sw = 190; set_rowh = 34;
    set_cx = x + set_sw + 1; set_cy = y; set_cw = w - set_sw - 1; set_ch = h;
    lfb_fill(x, y, w, h, PANEL_BG);
    settings_sidebar();
    settings_content();
}
void app_settings_click(int mx, int my) {
    if (mx < set_x + set_sw) {                       /* sidebar */
        for (int i = 0; i < 5; i++) {
            int ry = set_y + 56 + i * set_rowh;
            if (my >= ry && my < ry + set_rowh - 6) { set_cat = i; settings_sidebar(); settings_content(); return; }
        }
        return;
    }
    if (set_cat == 0) {                              /* Appearance: theme swatches */
        for (int i = 0; i < 5; i++)
            if (mx >= set_sw_x[i] && mx < set_sw_x[i] + set_sw_w &&
                my >= set_sw_y[i] && my < set_sw_y[i] + set_sw_h) {
                desktop_set_theme(THEMES[i].top, THEMES[i].bot, THEMES[i].acc);  /* repaints + reopens */
                return;
            }
    }
}
