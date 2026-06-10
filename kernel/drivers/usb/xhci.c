/* THUOS - xHCI USB host controller + USB-HID (keyboard & mouse).
 *
 * xHCI is the controller modern Intel/AMD laptops/PCs expose for USB (1/2/3);
 * legacy UHCI/OHCI/EHCI are gone on current hardware, so this is the one that
 * matters for real-hardware USB input.
 *
 *   Stage 1  discover (PCI) + BIOS->OS handoff + reset + cmd/event rings + run
 *   Stage 2  per-port: reset, Enable Slot, Address Device, read descriptors,
 *            Set Configuration
 *   Stage 3  HID boot protocol: configure the interrupt-IN endpoint, then
 *            usb_poll() turns reports into keyboard_inject()/mouse_inject()
 *
 * Verified in QEMU:  -device qemu-xhci -device usb-kbd -device usb-mouse
 *
 * DMA note: paging identity-maps the low 8 MiB and every ring/context/buffer
 * below is a static array living there, so virtual address == physical address
 * - which is what the controller DMAs against. */
#include "types.h"
#include "io.h"
#include "kprintf.h"
#include "vmm.h"
#include "keyboard.h"
#include "mouse.h"
#include "xhci.h"

/* ---------- PCI ---------- */
#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC
static uint32_t pci_r32(int b, int d, int f, int off) {
    outl(PCI_ADDR, 0x80000000u | (b << 16) | (d << 11) | (f << 8) | (off & 0xFC));
    return inl(PCI_DATA);
}
static void pci_w32(int b, int d, int f, int off, uint32_t v) {
    outl(PCI_ADDR, 0x80000000u | (b << 16) | (d << 11) | (f << 8) | (off & 0xFC));
    outl(PCI_DATA, v);
}

/* ---------- MMIO ---------- */
static inline uint32_t mr(uintptr_t base, uint32_t off) { return *(volatile uint32_t *)(base + off); }
static inline void     mw(uintptr_t base, uint32_t off, uint32_t v) { *(volatile uint32_t *)(base + off) = v; }
#define PHYS(p) ((uint32_t)(uintptr_t)(p))

/* operational regs */
#define OP_USBCMD 0x00
#define OP_USBSTS 0x04
#define OP_CRCR   0x18
#define OP_DCBAAP 0x30
#define OP_CONFIG 0x38
#define OP_PORTSC(p) (0x400 + ((p) - 1) * 0x10)
#define CMD_RS    (1u << 0)
#define CMD_HCRST (1u << 1)
#define STS_HCH   (1u << 0)
#define STS_CNR   (1u << 11)
/* runtime interrupter 0 */
#define IR0 0x20
#define IR_ERSTSZ 0x08
#define IR_ERSTBA 0x10
#define IR_ERDP   0x18
/* PORTSC */
#define P_CCS (1u << 0)
#define P_PED (1u << 1)
#define P_PR  (1u << 4)
#define P_PP  (1u << 9)
#define P_RW1C 0x00FE0000u            /* change bits 17..23 (write 1 to clear) */
/* TRB types */
#define TRB_NORMAL      1
#define TRB_SETUP       2
#define TRB_DATA        3
#define TRB_STATUS      4
#define TRB_LINK        6
#define TRB_ENABLE_SLOT 9
#define TRB_ADDRESS_DEV 11
#define TRB_CONFIG_EP   12
#define TRB_NOOP_CMD    23
#define TRB_TRANSFER_EVT   32
#define TRB_CMD_COMPLETE   33

typedef struct { volatile uint32_t lo, hi, status, control; } trb_t;
typedef struct { volatile uint64_t base; volatile uint32_t size, rsvd; } erst_t;

/* ---------- DMA structures ---------- */
#define CMD_TRBS 64
#define EVT_TRBS 64
#define MAX_SCRATCH 32
#define MAX_DEV 4
#define EP0_TRBS 16
#define EPI_TRBS 16
static uint64_t dcbaa[64]                  __attribute__((aligned(64)));
static trb_t    cmd_ring[CMD_TRBS]         __attribute__((aligned(64)));
static trb_t    evt_ring[EVT_TRBS]         __attribute__((aligned(64)));
static erst_t   erst[1]                    __attribute__((aligned(64)));
static uint64_t scratch_arr[MAX_SCRATCH]   __attribute__((aligned(4096)));
static uint8_t  scratch_buf[MAX_SCRATCH][4096] __attribute__((aligned(4096)));
static uint8_t  dev_ctx[MAX_DEV][2048]     __attribute__((aligned(64)));
static uint8_t  in_ctx[MAX_DEV][2112]      __attribute__((aligned(64)));
static trb_t    ep0_ring[MAX_DEV][EP0_TRBS] __attribute__((aligned(64)));
static trb_t    epi_ring[MAX_DEV][EPI_TRBS] __attribute__((aligned(64)));
static uint8_t  ctl_buf[MAX_DEV][512]      __attribute__((aligned(64)));
static uint8_t  hid_buf[MAX_DEV][8]        __attribute__((aligned(64)));

typedef struct {
    int      used, slot, port;
    uint32_t ep0_cyc, ep0_enq;
    uint32_t epi_cyc, epi_enq;
    int      is_kbd, is_mouse;
    uint8_t  ep_dci, ep_len;
    uint8_t  prev[8];
} dev_t;
static dev_t devs[MAX_DEV];

static uintptr_t capb, opb, rtb, dbb;
static uint32_t  maxports, maxslots, ctx_size;
static uint32_t  cmd_cyc = 1, cmd_enq = 0;
static uint32_t  evt_cyc = 1, evt_deq = 0;
static int       present = 0;
int xhci_present(void) { return present; }

/* ---------- HID usage -> ASCII (boot keyboard) ---------- */
static const char hid_map[128] = {
    [0x04]='a',[0x05]='b',[0x06]='c',[0x07]='d',[0x08]='e',[0x09]='f',[0x0a]='g',
    [0x0b]='h',[0x0c]='i',[0x0d]='j',[0x0e]='k',[0x0f]='l',[0x10]='m',[0x11]='n',
    [0x12]='o',[0x13]='p',[0x14]='q',[0x15]='r',[0x16]='s',[0x17]='t',[0x18]='u',
    [0x19]='v',[0x1a]='w',[0x1b]='x',[0x1c]='y',[0x1d]='z',
    [0x1e]='1',[0x1f]='2',[0x20]='3',[0x21]='4',[0x22]='5',[0x23]='6',[0x24]='7',
    [0x25]='8',[0x26]='9',[0x27]='0',
    [0x28]='\n',[0x29]=27,[0x2a]='\b',[0x2b]='\t',[0x2c]=' ',
    [0x2d]='-',[0x2e]='=',[0x2f]='[',[0x30]=']',[0x31]='\\',[0x33]=';',[0x34]='\'',
    [0x35]='`',[0x36]=',',[0x37]='.',[0x38]='/',
};
static const char hid_shift[128] = {
    [0x04]='A',[0x05]='B',[0x06]='C',[0x07]='D',[0x08]='E',[0x09]='F',[0x0a]='G',
    [0x0b]='H',[0x0c]='I',[0x0d]='J',[0x0e]='K',[0x0f]='L',[0x10]='M',[0x11]='N',
    [0x12]='O',[0x13]='P',[0x14]='Q',[0x15]='R',[0x16]='S',[0x17]='T',[0x18]='U',
    [0x19]='V',[0x1a]='W',[0x1b]='X',[0x1c]='Y',[0x1d]='Z',
    [0x1e]='!',[0x1f]='@',[0x20]='#',[0x21]='$',[0x22]='%',[0x23]='^',[0x24]='&',
    [0x25]='*',[0x26]='(',[0x27]=')',
    [0x28]='\n',[0x29]=27,[0x2a]='\b',[0x2b]='\t',[0x2c]=' ',
    [0x2d]='_',[0x2e]='+',[0x2f]='{',[0x30]='}',[0x31]='|',[0x33]=':',[0x34]='"',
    [0x35]='~',[0x36]='<',[0x37]='>',[0x38]='?',
};

/* ---------- low-level helpers ---------- */
static int wait_clear(uintptr_t b, uint32_t off, uint32_t m) {
    for (int i = 0; i < 2000000; i++) if (!(mr(b, off) & m)) return 1;
    return 0;
}
static int wait_set(uintptr_t b, uint32_t off, uint32_t m) {
    for (int i = 0; i < 2000000; i++) if (mr(b, off) & m) return 1;
    return 0;
}
static void ring_doorbell(uint32_t slot, uint32_t target) { mw(dbb, slot * 4, target); }

static void ring_link(trb_t *ring, int n, uint32_t cyc) {
    for (int i = 0; i < n; i++) { ring[i].lo = ring[i].hi = ring[i].status = ring[i].control = 0; }
    ring[n - 1].lo = PHYS(ring);
    ring[n - 1].control = (TRB_LINK << 10) | (1u << 1) | cyc;   /* Toggle Cycle */
}
/* push one TRB (control without the cycle bit), advancing enq/cycle over the link. */
static void ring_push(trb_t *ring, int n, uint32_t *enq, uint32_t *cyc,
                      uint32_t lo, uint32_t hi, uint32_t status, uint32_t ctl) {
    trb_t *t = &ring[*enq];
    t->lo = lo; t->hi = hi; t->status = status;
    t->control = ctl | *cyc;
    if (++(*enq) == n - 1) {
        ring[n - 1].control = (TRB_LINK << 10) | (1u << 1) | *cyc;  /* refresh link cycle */
        *enq = 0; *cyc ^= 1;
    }
}

/* Wait for an event of a specific type, skipping (and acking) others such as
 * Port Status Change events that share the ring. Returns 1 + completion code. */
static int wait_event(uint32_t want, uint32_t *cc, uint32_t *slot) {
    for (int i = 0; i < 3000000; i++) {
        trb_t *e = &evt_ring[evt_deq];
        if ((e->control & 1u) == evt_cyc) {
            uint32_t type = (e->control >> 10) & 0x3F;
            uint32_t c    = (e->status >> 24) & 0xFF;
            uint32_t s    = (e->control >> 24) & 0xFF;
            if (++evt_deq == EVT_TRBS) { evt_deq = 0; evt_cyc ^= 1; }
            mw(rtb, IR0 + IR_ERDP, PHYS(&evt_ring[evt_deq]) | (1u << 3));
            mw(rtb, IR0 + IR_ERDP + 4, 0);
            if (type == want) { if (cc) *cc = c; if (slot) *slot = s; return 1; }
            /* otherwise skip and keep waiting */
        }
    }
    return 0;
}
/* Submit a TRB on the command ring and wait for its Command Completion. */
static uint32_t cmd_submit(uint32_t lo, uint32_t hi, uint32_t ctl, uint32_t *out_slot) {
    ring_push(cmd_ring, CMD_TRBS, &cmd_enq, &cmd_cyc, lo, hi, 0, ctl);
    ring_doorbell(0, 0);
    uint32_t cc = 0, slot = 0;
    if (!wait_event(TRB_CMD_COMPLETE, &cc, &slot)) return 0;
    if (out_slot) *out_slot = slot;
    return cc ? cc : 1;
}

/* ---------- context field access ---------- */
static void cset(uint8_t *base, int ci, int dw, uint32_t v) {
    *(volatile uint32_t *)(base + ci * ctx_size + dw * 4) = v;
}
static uint32_t cget(uint8_t *base, int ci, int dw) {
    return *(volatile uint32_t *)(base + ci * ctx_size + dw * 4);
}
static void ctx_zero(uint8_t *base, int n) { for (int i = 0; i < n * (int)ctx_size; i++) base[i] = 0; }

/* control IN/OUT transfer on EP0; returns completion code (1 = success). */
static uint32_t control_xfer(dev_t *d, uint8_t bmRT, uint8_t bReq, uint16_t wVal,
                             uint16_t wIdx, uint16_t wLen, int dir_in) {
    uint32_t setup_lo = bmRT | (bReq << 8) | ((uint32_t)wVal << 16);
    uint32_t setup_hi = wIdx | ((uint32_t)wLen << 16);
    uint32_t trt = wLen ? (dir_in ? 3u : 2u) : 0u;
    ring_push(ep0_ring[d - devs], EP0_TRBS, &d->ep0_enq, &d->ep0_cyc,
              setup_lo, setup_hi, 8, (trt << 16) | (TRB_SETUP << 10) | (1u << 6));
    if (wLen) {
        ring_push(ep0_ring[d - devs], EP0_TRBS, &d->ep0_enq, &d->ep0_cyc,
                  PHYS(ctl_buf[d - devs]), 0, wLen,
                  ((dir_in ? 1u : 0u) << 16) | (TRB_DATA << 10));
    }
    ring_push(ep0_ring[d - devs], EP0_TRBS, &d->ep0_enq, &d->ep0_cyc,
              0, 0, 0, ((wLen && dir_in ? 0u : 1u) << 16) | (TRB_STATUS << 10) | (1u << 5));
    ring_doorbell(d->slot, 1);
    uint32_t cc = 0;
    if (!wait_event(TRB_TRANSFER_EVT, &cc, 0)) return 0;
    return cc ? cc : 1;
}

static void arm_interrupt(dev_t *d) {
    int idx = d - devs;
    ring_push(epi_ring[idx], EPI_TRBS, &d->epi_enq, &d->epi_cyc,
              PHYS(hid_buf[idx]), 0, d->ep_len, (TRB_NORMAL << 10) | (1u << 5) /*IOC*/);
    ring_doorbell(d->slot, d->ep_dci);
}

static int speed_mps0(uint32_t spd) { return spd == 4 ? 512 : spd == 3 ? 64 : 8; }

/* Reset/enable a root-hub port; returns its xHCI speed id, or 0 if it didn't enable. */
static uint32_t port_reset(uint32_t p) {
    uint32_t sc = mr(opb, OP_PORTSC(p));
    if (!(sc & P_PED)) {                                  /* USB2: needs a reset */
        mw(opb, OP_PORTSC(p), (sc & ~(P_RW1C | P_PED)) | P_PR);
        for (int i = 0; i < 1000000 && !(mr(opb, OP_PORTSC(p)) & P_PED); i++) {}
        sc = mr(opb, OP_PORTSC(p));
        mw(opb, OP_PORTSC(p), (sc & ~(P_RW1C | P_PED)) | P_RW1C);  /* clear change bits */
    }
    sc = mr(opb, OP_PORTSC(p));
    if (!(sc & P_PED)) return 0;
    return (sc >> 10) & 0xF;
}

static void enumerate(uint32_t port) {
    if ((unsigned)/*ndev*/0 >= MAX_DEV) return;
    int idx = -1;
    for (int i = 0; i < MAX_DEV; i++) if (!devs[i].used) { idx = i; break; }
    if (idx < 0) return;
    dev_t *d = &devs[idx];

    uint32_t spd = port_reset(port);
    if (!spd) return;

    uint32_t slot = 0;
    if (cmd_submit(0, 0, (TRB_ENABLE_SLOT << 10), &slot) != 1 || !slot) {
        kprintf("  [usb] port %u: Enable Slot failed\n", port); return;
    }

    /* device + input contexts */
    ctx_zero(dev_ctx[idx], 32);
    ctx_zero(in_ctx[idx], 33);
    dcbaa[slot] = PHYS(dev_ctx[idx]);

    ring_link(ep0_ring[idx], EP0_TRBS, 1);
    d->ep0_cyc = 1; d->ep0_enq = 0;

    /* Input Control Context: add slot (A0) + EP0 (A1) */
    cset(in_ctx[idx], 0, 1, 0x3);
    /* Slot context: context entries=1, speed, root port */
    cset(in_ctx[idx], 1, 0, (1u << 27) | (spd << 20));
    cset(in_ctx[idx], 1, 1, (port << 16));
    /* EP0 context: control, mps, TR dequeue|DCS, err count 3, avg trb len 8 */
    cset(in_ctx[idx], 2, 1, (3u << 1) | (4u << 3) | ((uint32_t)speed_mps0(spd) << 16));
    cset(in_ctx[idx], 2, 2, PHYS(ep0_ring[idx]) | 1u);
    cset(in_ctx[idx], 2, 3, 0);
    cset(in_ctx[idx], 2, 4, 8);

    d->slot = slot; d->port = port; d->used = 1;

    if (cmd_submit(PHYS(in_ctx[idx]), 0, (TRB_ADDRESS_DEV << 10) | (slot << 24), 0) != 1) {
        kprintf("  [usb] port %u: Address Device failed\n", port); d->used = 0; return;
    }

    /* GET_DESCRIPTOR(device, 18) */
    if (control_xfer(d, 0x80, 6, 0x0100, 0, 18, 1) != 1) {
        kprintf("  [usb] port %u: GET device descriptor failed\n", port); d->used = 0; return;
    }
    uint8_t *dd = ctl_buf[idx];
    uint16_t vid = dd[8] | (dd[9] << 8), pid = dd[10] | (dd[11] << 8);
    kprintf("  [usb] port %u: device %x:%x (USB %x.%x)\n", port, vid, pid, dd[3], dd[2]);

    /* GET_DESCRIPTOR(config): 9 bytes, then wTotalLength */
    if (control_xfer(d, 0x80, 6, 0x0200, 0, 9, 1) != 1) { d->used = 0; return; }
    uint16_t total = ctl_buf[idx][2] | (ctl_buf[idx][3] << 8);
    uint8_t  cfgval = ctl_buf[idx][5];
    if (total > sizeof ctl_buf[idx]) total = sizeof ctl_buf[idx];
    if (control_xfer(d, 0x80, 6, 0x0200, 0, total, 1) != 1) { d->used = 0; return; }

    /* parse interface + interrupt-IN endpoint */
    uint8_t *cfg = ctl_buf[idx];
    int iface = -1; d->is_kbd = d->is_mouse = 0; d->ep_dci = 0;
    for (uint32_t o = 0; o + 2 <= total; ) {
        uint8_t len = cfg[o], typ = cfg[o + 1];
        if (len == 0) break;
        if (typ == 4) {                              /* interface */
            iface = cfg[o + 2];
            if (cfg[o + 5] == 3) {                   /* HID class */
                if (cfg[o + 7] == 1) d->is_kbd = 1;
                else if (cfg[o + 7] == 2) d->is_mouse = 1;
            }
        } else if (typ == 5 && (d->is_kbd || d->is_mouse)) {   /* endpoint */
            uint8_t addr = cfg[o + 2], attr = cfg[o + 3];
            if ((addr & 0x80) && (attr & 0x3) == 3 && !d->ep_dci) {  /* interrupt IN */
                d->ep_dci = (addr & 0xF) * 2 + 1;
                d->ep_len = cfg[o + 4];
                if (d->ep_len == 0 || d->ep_len > 8) d->ep_len = 8;
            }
        }
        o += len;
    }
    if (!d->ep_dci) { kprintf("  [usb] port %u: not a HID boot device\n", port); d->used = 0; return; }

    /* SET_CONFIGURATION */
    if (control_xfer(d, 0x00, 9, cfgval, 0, 0, 0) != 1) { d->used = 0; return; }
    /* SET_PROTOCOL(boot=0) on the HID interface */
    control_xfer(d, 0x21, 0x0B, 0, iface < 0 ? 0 : iface, 0, 0);

    /* Configure the interrupt-IN endpoint */
    ring_link(epi_ring[idx], EPI_TRBS, 1);
    d->epi_cyc = 1; d->epi_enq = 0;
    ctx_zero(in_ctx[idx], 33);
    cset(in_ctx[idx], 0, 1, 0x1 | (1u << d->ep_dci));        /* add slot + the EP */
    cset(in_ctx[idx], 1, 0, ((uint32_t)d->ep_dci << 27) | (spd << 20));
    cset(in_ctx[idx], 1, 1, (port << 16));
    cset(in_ctx[idx], d->ep_dci + 1, 0, (3u << 16));         /* interval ~ */
    cset(in_ctx[idx], d->ep_dci + 1, 1, (3u << 1) | (7u << 3) | ((uint32_t)d->ep_len << 16)); /* Int IN */
    cset(in_ctx[idx], d->ep_dci + 1, 2, PHYS(epi_ring[idx]) | 1u);
    cset(in_ctx[idx], d->ep_dci + 1, 4, d->ep_len);
    if (cmd_submit(PHYS(in_ctx[idx]), 0, (TRB_CONFIG_EP << 10) | (slot << 24), 0) != 1) {
        kprintf("  [usb] port %u: Configure Endpoint failed\n", port); d->used = 0; return;
    }

    kprintf("  [usb] port %u: %s ready (slot %u, ep dci %u)\n", port,
            d->is_kbd ? "keyboard" : "mouse", slot, d->ep_dci);
    arm_interrupt(d);
}

static int in_prev(dev_t *d, uint8_t k) {
    for (int i = 2; i < 8; i++) if (d->prev[i] == k) return 1;
    return 0;
}
static void hid_handle(dev_t *d) {
    uint8_t *r = hid_buf[d - devs];
    if (d->is_kbd) {
        int shift = (r[0] & 0x22) ? 1 : 0;
        for (int i = 2; i < 8; i++) {
            uint8_t k = r[i];
            if (k && k < 128 && !in_prev(d, k)) {
                char c = shift ? hid_shift[k] : hid_map[k];
                if (c) keyboard_inject(c);
            }
        }
        for (int i = 0; i < 8; i++) d->prev[i] = r[i];
    } else if (d->is_mouse) {
        mouse_inject((int)(int8_t)r[1], (int)(int8_t)r[2], r[0]);
    }
}

void usb_poll(void) {
    if (!present) return;
    uint32_t type, cc, slot;
    for (int guard = 0; guard < 64; guard++) {
        trb_t *e = &evt_ring[evt_deq];
        if ((e->control & 1u) != evt_cyc) break;
        type = (e->control >> 10) & 0x3F;
        cc   = (e->status >> 24) & 0xFF; (void)cc;
        slot = (e->control >> 24) & 0xFF;
        if (++evt_deq == EVT_TRBS) { evt_deq = 0; evt_cyc ^= 1; }
        mw(rtb, IR0 + IR_ERDP, PHYS(&evt_ring[evt_deq]) | (1u << 3));
        mw(rtb, IR0 + IR_ERDP + 4, 0);
        if (type == TRB_TRANSFER_EVT) {
            for (int i = 0; i < MAX_DEV; i++) {
                if (devs[i].used && devs[i].slot == (int)slot && devs[i].ep_dci) {
                    hid_handle(&devs[i]);
                    arm_interrupt(&devs[i]);
                    break;
                }
            }
        }
    }
}

/* ---------- discovery / bring-up (stage 1) ---------- */
static void take_ownership(uint32_t hccparams1) {
    uint32_t xecp = (hccparams1 >> 16) & 0xFFFF;
    if (!xecp) return;
    uintptr_t p = capb + xecp * 4;
    for (int g = 0; g < 64; g++) {
        uint32_t cap = mr(p, 0);
        uint8_t id = cap & 0xFF, next = (cap >> 8) & 0xFF;
        if (id == 1) {
            mw(p, 0, cap | (1u << 24));
            if (!wait_clear(p, 0, (1u << 16))) kprintf("  [usb] BIOS did not release controller\n");
            return;
        }
        if (!next) break;
        p += next * 4;
    }
}
static uint32_t pci_find_xhci(void) {
    uint32_t bar = 0;
    for (int d = 0; d < 32; d++) for (int f = 0; f < 8; f++) {
        uint32_t id = pci_r32(0, d, f, 0x00);
        if (id == 0xFFFFFFFFu) { if (f == 0) break; else continue; }
        uint32_t cls = pci_r32(0, d, f, 0x08);
        if ((cls >> 24) != 0x0C || ((cls >> 16) & 0xFF) != 0x03) continue;
        uint32_t pi = (cls >> 8) & 0xFF;
        kprintf("  [usb] %s controller at %02d:%d\n",
                pi == 0x30 ? "xHCI" : pi == 0x20 ? "EHCI" : pi == 0x10 ? "OHCI" : "UHCI", d, f);
        if (pi == 0x30 && !bar) {
            bar = pci_r32(0, d, f, 0x10) & ~0xFu;
            pci_w32(0, d, f, 0x04, pci_r32(0, d, f, 0x04) | 0x6);   /* MEM + bus master */
        }
    }
    return bar;
}

void xhci_init(void) {
    uint32_t bar = pci_find_xhci();
    if (!bar) { kprintf("  [usb] no xHCI controller found\n"); return; }
    if (!vmm_map_mmio(bar, 0x10000)) { kprintf("  [usb] could not map xHCI MMIO\n"); return; }
    capb = (uintptr_t)bar;

    uint8_t  caplen = *(volatile uint8_t *)capb;
    uint32_t hcs1 = mr(capb, 0x04), hcs2 = mr(capb, 0x08), hcc1 = mr(capb, 0x10);
    maxslots = hcs1 & 0xFF;
    maxports = (hcs1 >> 24) & 0xFF;
    ctx_size = (hcc1 & (1u << 2)) ? 64 : 32;
    opb = capb + caplen;
    rtb = capb + (mr(capb, 0x18) & ~0x1Fu);
    dbb = capb + (mr(capb, 0x14) & ~0x3u);
    kprintf("  [usb] xHCI: %u ports, %u slots, %u-byte contexts\n", maxports, maxslots, ctx_size);

    take_ownership(hcc1);

    mw(opb, OP_USBCMD, mr(opb, OP_USBCMD) & ~CMD_RS);
    if (!wait_set(opb, OP_USBSTS, STS_HCH)) { kprintf("  [usb] no halt\n"); return; }
    mw(opb, OP_USBCMD, CMD_HCRST);
    if (!wait_clear(opb, OP_USBCMD, CMD_HCRST) || !wait_clear(opb, OP_USBSTS, STS_CNR)) {
        kprintf("  [usb] reset timeout\n"); return;
    }

    uint32_t slots_en = maxslots > 63 ? 63 : maxslots;
    mw(opb, OP_CONFIG, slots_en);
    for (uint32_t i = 0; i < 64; i++) dcbaa[i] = 0;
    uint32_t nscratch = ((hcs2 >> 21) & 0x1F) | (((hcs2 >> 27) & 0x1F) << 5);
    if (nscratch) {
        if (nscratch > MAX_SCRATCH) nscratch = MAX_SCRATCH;
        for (uint32_t i = 0; i < nscratch; i++) scratch_arr[i] = PHYS(scratch_buf[i]);
        dcbaa[0] = PHYS(scratch_arr);
    }
    mw(opb, OP_DCBAAP, PHYS(dcbaa)); mw(opb, OP_DCBAAP + 4, 0);

    ring_link(cmd_ring, CMD_TRBS, 1);
    cmd_enq = 0; cmd_cyc = 1;
    mw(opb, OP_CRCR, PHYS(cmd_ring) | 1u); mw(opb, OP_CRCR + 4, 0);

    for (uint32_t i = 0; i < EVT_TRBS; i++) { evt_ring[i].lo = evt_ring[i].hi = evt_ring[i].status = evt_ring[i].control = 0; }
    evt_deq = 0; evt_cyc = 1;
    erst[0].base = PHYS(evt_ring); erst[0].size = EVT_TRBS; erst[0].rsvd = 0;
    mw(rtb, IR0 + IR_ERSTSZ, 1);
    mw(rtb, IR0 + IR_ERDP, PHYS(evt_ring)); mw(rtb, IR0 + IR_ERDP + 4, 0);
    mw(rtb, IR0 + IR_ERSTBA, PHYS(erst)); mw(rtb, IR0 + IR_ERSTBA + 4, 0);

    mw(opb, OP_USBCMD, mr(opb, OP_USBCMD) | CMD_RS);
    if (!wait_clear(opb, OP_USBSTS, STS_HCH)) { kprintf("  [usb] no start\n"); return; }

    if (cmd_submit(0, 0, (TRB_NOOP_CMD << 10), 0) != 1) {
        kprintf("  [usb] No-Op failed; controller not usable\n"); return;
    }
    present = 1;

    /* Enumerate every connected port (stage 2/3). */
    int found = 0;
    for (uint32_t p = 1; p <= maxports; p++) {
        if (mr(opb, OP_PORTSC(p)) & P_CCS) { enumerate(p); found = 1; }
    }
    if (!found) kprintf("  [usb] no devices attached\n");
}
