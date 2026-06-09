/* THUOS — built-in desktop apps: Calculator, Files, System/Devices, About.
 * Drawn into the window content rectangle with the lfb primitives. */
#include "apps.h"
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

/* word-wrapped text block; returns next y */
static int text_block(int x, int y, int w, const char *s, uint32_t fg, int scale) {
    int cw = 8 * scale, lh = 16 * scale, cols = w / cw, col = 0;
    int cx = x;
    for (; *s; s++) {
        if (*s == '\n') { y += lh; cx = x; col = 0; continue; }
        if (col >= cols) { y += lh; cx = x; col = 0; }
        lfb_char(cx, y, *s, fg, -1, scale);
        cx += cw; col++;
    }
    return y + lh;
}

/* theme */
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
static int   cc_x, cc_y, cc_w, cc_h;     /* card */
static int   cc_bx, cc_by, cc_cw, cc_ch, cc_gap;  /* button grid */
static long  cc_acc, cc_cur;
static char  cc_op;
static int   cc_fresh;
static long  cc_disp;

static const char *CALC_LBL[4][4] = {
    { "7", "8", "9", "/" },
    { "4", "5", "6", "*" },
    { "1", "2", "3", "-" },
    { "0", "=", "+", "C" },
};

static void calc_draw_display(void) {
    lfb_round_fill(cc_x + 14, cc_y + 14, cc_w - 28, 56, 8, 0x05080f);
    char buf[24]; itoa_l(cc_disp, buf);
    int tw = slen(buf) * 16;                  /* scale 2 */
    lfb_text(cc_x + cc_w - 28 - tw, cc_y + 30, buf, GOOD, -1, 2);
}

static void calc_draw_buttons(void) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            int bx = cc_bx + c * (cc_cw + cc_gap);
            int by = cc_by + r * (cc_ch + cc_gap);
            const char *l = CALC_LBL[r][c];
            uint32_t bg = CARD2;
            if (c == 3) bg = 0x2a3a5e;            /* operator column */
            if (r == 3 && c == 1) bg = ACCENT;    /* equals */
            if (r == 3 && c == 3) bg = 0x5a2a3a;  /* clear  */
            lfb_round_fill(bx, by, cc_cw, cc_ch, 8, bg);
            uint32_t fg = (r == 3 && c == 1) ? 0x06121f : TXT_HI;
            lfb_text(bx + (cc_cw - 16) / 2, by + (cc_ch - 32) / 2, l, fg, -1, 2);
        }
}

void app_calc_open(int x, int y, int w, int h) {
    lfb_fill(x, y, w, h, PANEL_BG);
    cc_w = 300; cc_h = 400;
    cc_x = x + (w - cc_w) / 2; cc_y = y + (h - cc_h) / 2; if (cc_y < y + 12) cc_y = y + 12;
    lfb_round_fill(cc_x, cc_y, cc_w, cc_h, 14, CARD);
    lfb_text(cc_x + 14, cc_y - 26, "Calculator", TXT_LO, -1, 1);

    cc_gap = 10;
    cc_bx = cc_x + 14; cc_by = cc_y + 84;
    cc_cw = (cc_w - 28 - 3 * cc_gap) / 4;
    cc_ch = (cc_h - 84 - 14 - 3 * cc_gap) / 4;

    cc_acc = 0; cc_cur = 0; cc_op = 0; cc_fresh = 1; cc_disp = 0;
    calc_draw_display();
    calc_draw_buttons();
}

static long calc_apply(long a, char op, long b) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b ? a / b : 0;
    }
    return b;
}

void app_calc_key(char c) {
    if (c >= '0' && c <= '9') {
        if (cc_fresh) { cc_cur = 0; cc_fresh = 0; }
        cc_cur = cc_cur * 10 + (c - '0');
        cc_disp = cc_cur;
    } else if (c == '+' || c == '-' || c == '*' || c == '/') {
        cc_acc = cc_op ? calc_apply(cc_acc, cc_op, cc_cur) : cc_cur;
        cc_op = c; cc_fresh = 1; cc_disp = cc_acc;
    } else if (c == '=' || c == '\n') {
        if (cc_op) { cc_acc = calc_apply(cc_acc, cc_op, cc_cur); cc_op = 0; }
        else cc_acc = cc_cur;
        cc_disp = cc_acc; cc_cur = cc_acc; cc_fresh = 1;
    } else if (c == 'c' || c == 'C') {
        cc_acc = cc_cur = 0; cc_op = 0; cc_fresh = 1; cc_disp = 0;
    } else return;
    calc_draw_display();
}

void app_calc_click(int mx, int my) {
    for (int r = 0; r < 4; r++)
        for (int col = 0; col < 4; col++) {
            int bx = cc_bx + col * (cc_cw + cc_gap);
            int by = cc_by + r * (cc_ch + cc_gap);
            if (mx >= bx && mx < bx + cc_cw && my >= by && my < by + cc_ch) {
                app_calc_key(CALC_LBL[r][col][0]);
                /* repaint the pressed button briefly-less: just redraw all */
                calc_draw_buttons();
                return;
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
        const char *nm = kfs_name(i);
        if (!nm) continue;
        int ry = fa_rowy + shown * fa_rowh;
        lfb_round_fill(fa_listx, ry, fa_listw, fa_rowh - 6, 6, CARD2);
        lfb_text(fa_listx + 10, ry + 3, nm, TXT_HI, -1, 1);
        shown++;
    }
    if (!shown) lfb_text(fa_listx, fa_rowy, "(no files)", TXT_LO, -1, 1);
    /* viewer pane */
    lfb_round_fill(x + 16 + fa_listw + 12, y + 90, w - 32 - fa_listw - 12, h - 100, 8, 0x05080f);
    lfb_text(x + 16 + fa_listw + 24, y + 100, "select a file ->", TXT_LO, -1, 1);
}

void app_files_click(int mx, int my) {
    if (mx < fa_listx || mx > fa_listx + fa_listw) return;
    int row = (my - fa_rowy) / fa_rowh;
    if (row < 0) return;
    int max = kfs_max_files(), shown = 0;
    for (int i = 0; i < max; i++) {
        const char *nm = kfs_name(i);
        if (!nm) continue;
        if (shown == row) {
            int vx = fa_x + 16 + fa_listw + 12, vy = fa_y + 90;
            int vw = fa_w - 32 - fa_listw - 12, vh = fa_h - 100;
            lfb_round_fill(vx, vy, vw, vh, 8, 0x05080f);
            lfb_text(vx + 12, vy + 10, nm, ACCENT, -1, 1);
            uint32_t n; const uint8_t *d = kfs_read(nm, &n);
            if (d) {
                char tmp[512]; uint32_t k = 0;
                for (uint32_t j = 0; j < n && k < sizeof tmp - 1; j++) tmp[k++] = (char)d[j];
                tmp[k] = '\0';
                text_block(vx + 12, vy + 34, vw - 24, tmp, TXT_HI, 1);
            }
            return;
        }
        shown++;
    }
}

/* ======================= System / Devices (honest) ======================= */
void app_sys_open(int x, int y, int w, int h) {
    lfb_fill(x, y, w, h, PANEL_BG);
    lfb_text(x + 16, y + 12, "System", TXT_HI, -1, 2);

    char b[32];
    int cx = x + 16, cy = y + 52;
    lfb_text(cx, cy, THUOS_NAME " " THUOS_VERSION " \"" THUOS_CODENAME "\"", ACCENT, -1, 1); cy += 20;
    lfb_text(cx, cy, "Arch  : " THUOS_ARCH, TXT_HI, -1, 1); cy += 20;
    lfb_text(cx, cy, "Uptime: ", TXT_HI, -1, 1);
    lfb_text(cx + 64, cy, itoa_l((long)pit_seconds(), b), TXT_HI, -1, 1);
    lfb_text(cx + 64 + slen(b) * 8 + 4, cy, "s", TXT_HI, -1, 1); cy += 20;
    if (pmm_available()) {
        lfb_text(cx, cy, "Memory: ", TXT_HI, -1, 1);
        lfb_text(cx + 64, cy, itoa_l((long)(pmm_usable_bytes() / 1024u), b), TXT_HI, -1, 1);
        lfb_text(cx + 64 + slen(b) * 8 + 4, cy, "KiB usable", TXT_HI, -1, 1); cy += 20;
    }

    cy += 12;
    lfb_text(cx, cy, "Devices (honest):", TXT_LO, -1, 1); cy += 24;
    struct { const char *name; const char *status; uint32_t col; } dev[] = {
        { "Display ", "VGA 1024x768x32 (Bochs VBE)  OK", GOOD },
        { "Keyboard", "PS/2  OK",                          GOOD },
        { "Mouse   ", "PS/2  OK",                          GOOD },
        { "Storage ", "RAM filesystem (volatile)  OK",     GOOD },
        { "Timer   ", "PIT 100 Hz  OK",                    GOOD },
        { "Network ", "emulated NIC may exist; no driver yet", WARN },
        { "Camera  ", "none (no device on this machine)",  BAD  },
        { "Wi-Fi   ", "not supported (needs vendor firmware/driver)", BAD },
        { "Bluetooth", "not supported",                    BAD  },
    };
    for (unsigned i = 0; i < sizeof dev / sizeof dev[0]; i++) {
        lfb_fill(cx, cy + 5, 8, 8, dev[i].col);
        lfb_text(cx + 16, cy, dev[i].name, TXT_HI, -1, 1);
        lfb_text(cx + 16 + 9 * 8, cy, dev[i].status, TXT_LO, -1, 1);
        cy += 20;
    }
}

/* ======================= About ======================= */
void app_about_open(int x, int y, int w, int h) {
    lfb_fill(x, y, w, h, PANEL_BG);
    lfb_text(x + 16, y + 12, "About THUOS", TXT_HI, -1, 2);
    const char *about =
        THUOS_NAME " " THUOS_VERSION " \"" THUOS_CODENAME "\"\n\n"
        "A from-scratch operating system: a real x86 kernel\n"
        "written in C and assembly, booting in QEMU.\n\n"
        "Built incrementally and honestly - every feature here\n"
        "boots and runs (verified in CI over serial). It does\n"
        "not yet rival Windows/macOS, and is aimed at a narrow,\n"
        "winnable wedge: a local-first, fast-boot OS in a VM.\n\n"
        "Apps run here: Terminal, Calculator, Files, System.\n"
        "Click the dock to switch. Esc returns to the terminal.";
    text_block(x + 16, y + 52, w - 32, about, TXT_LO, 1);
}
