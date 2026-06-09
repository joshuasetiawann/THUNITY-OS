/* THUOS — high-resolution linear framebuffer. See lfb.h. */
#include "lfb.h"
#include "io.h"
#include "vmm.h"
#include "gfx.h"   /* gfx_glyph(): the 8x16 VGA font extracted at boot */

/* ---- PCI: find the display controller's linear framebuffer (BAR0) ---- */
static uint32_t pci_cfg_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off) {
    uint32_t addr = 0x80000000u | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                    ((uint32_t)func << 8) | (off & 0xFC);
    outl(0xCF8, addr);
    return inl(0xCFC);
}

static uint32_t pci_find_vga_lfb(void) {
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t id = pci_cfg_read(0, slot, 0, 0x00);
        if (id == 0xFFFFFFFFu) continue;
        uint32_t cls = pci_cfg_read(0, slot, 0, 0x08);
        if ((cls >> 24) == 0x03) {                       /* class 0x03 = display */
            uint32_t bar0 = pci_cfg_read(0, slot, 0, 0x10);
            if (!(bar0 & 1))                             /* memory BAR */
                return bar0 & 0xFFFFFFF0u;
        }
    }
    return 0xFD000000u;   /* QEMU std-vga default, as a fallback */
}

/* ---- Bochs VBE / DISPI ---- */
#define VBE_INDEX 0x01CE
#define VBE_DATA  0x01CF
static void dispi_write(uint16_t idx, uint16_t val) { outw(VBE_INDEX, idx); outw(VBE_DATA, val); }

#define DISPI_XRES    1
#define DISPI_YRES    2
#define DISPI_BPP     3
#define DISPI_ENABLE  4
#define DISPI_ENABLED 0x01
#define DISPI_LFB     0x40

static volatile uint32_t *fbp;
static int W, H, pitch_px, active;

int lfb_init(int w, int h) {
    uint32_t lfb = pci_find_vga_lfb();
    if (!lfb) return 0;
    if (!vmm_map_lfb(lfb, (uint32_t)w * (uint32_t)h * 4u + 0x1000u)) return 0;

    dispi_write(DISPI_ENABLE, 0);
    dispi_write(DISPI_XRES, (uint16_t)w);
    dispi_write(DISPI_YRES, (uint16_t)h);
    dispi_write(DISPI_BPP, 32);
    dispi_write(DISPI_ENABLE, DISPI_ENABLED | DISPI_LFB);

    fbp = (volatile uint32_t *)(uintptr_t)lfb;
    W = w; H = h; pitch_px = w;
    active = 1;
    lfb_clear(0x000000);
    return 1;
}

int lfb_active(void) { return active; }
int lfb_width(void)  { return W; }
int lfb_height(void) { return H; }

void lfb_pixel(int x, int y, uint32_t rgb) {
    if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) fbp[y * pitch_px + x] = rgb;
}

void lfb_clear(uint32_t rgb) {
    int n = W * H;
    for (int i = 0; i < n; i++) fbp[i] = rgb;
}

void lfb_fill(int x, int y, int w, int h, uint32_t rgb) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > W) w = W - x;
    if (y + h > H) h = H - y;
    for (int j = 0; j < h; j++) {
        volatile uint32_t *row = fbp + (y + j) * pitch_px + x;
        for (int i = 0; i < w; i++) row[i] = rgb;
    }
}

static uint32_t lerp(uint32_t a, uint32_t b, int num, int den) {
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
    int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF;
    int r=ar+(br-ar)*num/den, g=ag+(bg-ag)*num/den, bl=ab+(bb-ab)*num/den;
    return ((uint32_t)r<<16)|((uint32_t)g<<8)|(uint32_t)bl;
}

void lfb_gradient_v(int x, int y, int w, int h, uint32_t top, uint32_t bot) {
    for (int j = 0; j < h; j++) lfb_fill(x, y + j, w, 1, lerp(top, bot, j, h > 1 ? h - 1 : 1));
}

void lfb_rect(int x, int y, int w, int h, uint32_t rgb) {
    lfb_fill(x, y, w, 1, rgb); lfb_fill(x, y + h - 1, w, 1, rgb);
    lfb_fill(x, y, 1, h, rgb); lfb_fill(x + w - 1, y, 1, h, rgb);
}

static int isqrt(int n) { int x = 0; while ((x + 1) * (x + 1) <= n) x++; return x; }

static int corner_inset(int j, int h, int r) {
    int dy = -1;
    if (j < r)            dy = r - j;
    else if (j >= h - r)  dy = r - (h - 1 - j);
    if (dy < 0) return 0;
    return r - isqrt(r * r - dy * dy);
}

void lfb_round_fill(int x, int y, int w, int h, int r, uint32_t rgb) {
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    for (int j = 0; j < h; j++) {
        int in = corner_inset(j, h, r);
        lfb_fill(x + in, y + j, w - 2 * in, 1, rgb);
    }
}

void lfb_disc(int cx, int cy, int r, uint32_t rgb) {
    for (int dy = -r; dy <= r; dy++) {
        int half = isqrt(r * r - dy * dy);
        lfb_fill(cx - half, cy + dy, 2 * half + 1, 1, rgb);
    }
}

void lfb_line(int x0, int y0, int x1, int y1, uint32_t rgb) {
    int dx = x1 - x0, dy = y1 - y0;
    int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1;
    int err = (adx > ady ? adx : -ady) / 2, e2;
    for (;;) {
        lfb_pixel(x0, y0, rgb);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -adx) { err -= ady; x0 += sx; }
        if (e2 <  ady) { err += adx; y0 += sy; }
    }
}

void lfb_char(int x, int y, char ch, uint32_t fg, long bg, int s) {
    const uint8_t *g = gfx_glyph((uint8_t)ch);
    for (int r = 0; r < 16; r++) {
        uint8_t bits = g[r];
        for (int c = 0; c < 8; c++) {
            if (bits & (0x80 >> c))      lfb_fill(x + c * s, y + r * s, s, s, fg);
            else if (bg >= 0)            lfb_fill(x + c * s, y + r * s, s, s, (uint32_t)bg);
        }
    }
}

void lfb_text(int x, int y, const char *str, uint32_t fg, long bg, int s) {
    for (; *str; str++, x += 8 * s) lfb_char(x, y, *str, fg, bg, s);
}

void lfb_scroll_up(int x, int y, int w, int h, int dy, uint32_t fill) {
    for (int j = 0; j < h - dy; j++) {
        volatile uint32_t *d = fbp + (y + j) * pitch_px + x;
        volatile uint32_t *s = fbp + (y + j + dy) * pitch_px + x;
        for (int i = 0; i < w; i++) d[i] = s[i];
    }
    lfb_fill(x, y + h - dy, w, dy, fill);
}

void lfb_blit_get(int x, int y, int w, int h, uint32_t *buf) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int xx = x + i, yy = y + j;
            buf[j * w + i] = ((unsigned)xx < (unsigned)W && (unsigned)yy < (unsigned)H)
                             ? fbp[yy * pitch_px + xx] : 0;
        }
}

void lfb_blit_put(int x, int y, int w, int h, const uint32_t *buf) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int xx = x + i, yy = y + j;
            if ((unsigned)xx < (unsigned)W && (unsigned)yy < (unsigned)H)
                fbp[yy * pitch_px + xx] = buf[j * w + i];
        }
}
