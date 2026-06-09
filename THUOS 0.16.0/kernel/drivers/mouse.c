/* THUOS — PS/2 mouse driver. See mouse.h. */
#include "mouse.h"
#include "irq.h"
#include "io.h"

#define PS2_DATA 0x60
#define PS2_CMD  0x64

static volatile int gx, gy, gbtn, gdirty;
static int sw, sh;
static uint8_t pkt[3];
static int idx;

static void ps2_wait_write(void) { for (int i = 0; i < 100000; i++) if (!(inb(PS2_CMD) & 2)) return; }
static void ps2_wait_read(void)  { for (int i = 0; i < 100000; i++) if (inb(PS2_CMD) & 1)  return; }

static void mouse_cmd(uint8_t v) {
    ps2_wait_write(); outb(PS2_CMD, 0xD4);     /* next byte -> mouse */
    ps2_wait_write(); outb(PS2_DATA, v);
    ps2_wait_read();  (void)inb(PS2_DATA);     /* eat ACK (0xFA) */
}

static void mouse_cb(registers_t *r) {
    (void)r;
    uint8_t b = inb(PS2_DATA);
    if (idx == 0 && !(b & 0x08)) return;       /* resync: bit3 of byte0 is always 1 */
    pkt[idx++] = b;
    if (idx < 3) return;
    idx = 0;

    int dx = (int)pkt[1] - ((pkt[0] & 0x10) ? 256 : 0);
    int dy = (int)pkt[2] - ((pkt[0] & 0x20) ? 256 : 0);
    gx += dx;
    gy -= dy;                                  /* screen y grows downward */
    if (gx < 0)      gx = 0;
    if (gx > sw - 1) gx = sw - 1;
    if (gy < 0)      gy = 0;
    if (gy > sh - 1) gy = sh - 1;
    gbtn = pkt[0] & 0x07;
    gdirty = 1;
}

void mouse_init(int screen_w, int screen_h) {
    sw = screen_w; sh = screen_h;
    gx = sw / 2; gy = sh / 2; gbtn = 0; idx = 0; gdirty = 1;

    ps2_wait_write(); outb(PS2_CMD, 0xA8);     /* enable auxiliary (mouse) port */

    ps2_wait_write(); outb(PS2_CMD, 0x20);     /* read controller config byte */
    ps2_wait_read();  uint8_t cfg = inb(PS2_DATA);
    cfg |=  0x02;                              /* enable IRQ12               */
    cfg &= ~0x20;                              /* enable mouse clock         */
    ps2_wait_write(); outb(PS2_CMD, 0x60);
    ps2_wait_write(); outb(PS2_DATA, cfg);

    mouse_cmd(0xF6);                           /* set defaults               */
    mouse_cmd(0xF4);                           /* enable data reporting      */

    uint8_t m2 = inb(0xA1); outb(0xA1, (uint8_t)(m2 & ~(1 << 4)));  /* unmask IRQ12  */
    uint8_t m1 = inb(0x21); outb(0x21, (uint8_t)(m1 & ~(1 << 2)));  /* unmask cascade */

    irq_register_handler(12, mouse_cb);
}

int mouse_x(void)    { return gx; }
int mouse_y(void)    { return gy; }
int mouse_left(void) { return gbtn & 1; }

int mouse_take_event(void) {
    if (!gdirty) return 0;
    gdirty = 0;
    return 1;
}
