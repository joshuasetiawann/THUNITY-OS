/* THUOS - xHCI USB host controller, stage 1: discovery + bring-up + ports.
 *
 * This is the controller modern Intel/AMD laptops/PCs expose for USB (1/2/3);
 * the legacy UHCI/OHCI/EHCI controllers are gone on current hardware, so xHCI
 * is the one that matters for real-hardware USB input. Stage 1 gets the
 * controller running and proves the data path; device enumeration (stage 2) and
 * HID keyboard/mouse (stage 3) build on top.
 *
 * Verified in QEMU:  -device qemu-xhci -device usb-kbd -device usb-mouse
 *
 * DMA note: paging identity-maps the low 8 MiB, and all the rings/contexts below
 * are static arrays that live there, so their virtual address equals their
 * physical address - which is what the controller DMAs against. */
#include "types.h"
#include "io.h"
#include "kprintf.h"
#include "vmm.h"
#include "xhci.h"

/* ---------- PCI config space ---------- */
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

/* ---------- MMIO helpers (byte offsets) ---------- */
static inline uint32_t mr(uintptr_t base, uint32_t off) { return *(volatile uint32_t *)(base + off); }
static inline void     mw(uintptr_t base, uint32_t off, uint32_t v) { *(volatile uint32_t *)(base + off) = v; }
#define PHYS(p) ((uint32_t)(uintptr_t)(p))

/* ---------- register offsets ---------- */
/* operational */
#define OP_USBCMD   0x00
#define OP_USBSTS   0x04
#define OP_CRCR     0x18
#define OP_DCBAAP   0x30
#define OP_CONFIG   0x38
#define OP_PORTSC(p) (0x400 + ((p) - 1) * 0x10)   /* p = 1..maxports */
/* USBCMD/USBSTS bits */
#define CMD_RS    (1u << 0)
#define CMD_HCRST (1u << 1)
#define STS_HCH   (1u << 0)
#define STS_CNR   (1u << 11)
/* runtime interrupter 0 (rt + 0x20) */
#define IR0       0x20
#define IR_ERSTSZ 0x08
#define IR_ERSTBA 0x10
#define IR_ERDP   0x18
/* TRB types */
#define TRB_NOOP_CMD 23
#define TRB_CMD_COMPLETION 33

typedef struct { volatile uint32_t lo, hi, status, control; } trb_t;       /* 16 bytes */
typedef struct { volatile uint64_t base; volatile uint32_t size, rsvd; } erst_t;

/* ---------- DMA structures (in low, identity-mapped RAM) ---------- */
#define CMD_TRBS 64
#define EVT_TRBS 64
#define MAX_SCRATCH 32
static uint64_t dcbaa[64]                 __attribute__((aligned(64)));
static trb_t    cmd_ring[CMD_TRBS]        __attribute__((aligned(64)));
static trb_t    evt_ring[EVT_TRBS]        __attribute__((aligned(64)));
static erst_t   erst[1]                   __attribute__((aligned(64)));
static uint64_t scratch_arr[MAX_SCRATCH]  __attribute__((aligned(4096)));
static uint8_t  scratch_buf[MAX_SCRATCH][4096] __attribute__((aligned(4096)));

static uintptr_t capb, opb, rtb, dbb;
static uint32_t  maxports, maxslots;
static uint32_t  cmd_cycle = 1, cmd_enq = 0;
static uint32_t  evt_cycle = 1, evt_deq = 0;
static int       present = 0;

int xhci_present(void) { return present; }

static int wait_clear(uintptr_t base, uint32_t off, uint32_t mask) {
    for (int i = 0; i < 2000000; i++) if (!(mr(base, off) & mask)) return 1;
    return 0;
}
static int wait_set(uintptr_t base, uint32_t off, uint32_t mask) {
    for (int i = 0; i < 2000000; i++) if (mr(base, off) & mask) return 1;
    return 0;
}

/* Some firmware (real machines) owns the controller at boot; request ownership
 * via the USBLEGSUP extended capability before we touch it. No-op on QEMU. */
static void take_ownership(uint32_t hccparams1) {
    uint32_t xecp = (hccparams1 >> 16) & 0xFFFF;     /* in 32-bit dwords from cap base */
    if (!xecp) return;
    uintptr_t p = capb + xecp * 4;
    for (int guard = 0; guard < 64; guard++) {
        uint32_t cap = mr(p, 0);
        uint8_t  id  = cap & 0xFF;
        uint8_t  next = (cap >> 8) & 0xFF;
        if (id == 1) {                                /* USBLEGSUP */
            mw(p, 0, cap | (1u << 24));               /* set OS-owned */
            if (!wait_clear(p, 0, (1u << 16)))        /* wait BIOS-owned (bit16) clear */
                kprintf("  [usb] warning: BIOS did not release the controller\n");
            return;
        }
        if (!next) break;
        p += next * 4;
    }
}

/* Find an xHCI controller (class 0C, subclass 03, prog-if 30). Reports the other
 * USB controller types if present, for honest diagnostics. Returns its BAR phys
 * (low 32 bits) or 0. */
static uint32_t pci_find_xhci(void) {
    uint32_t bar = 0;
    for (int d = 0; d < 32; d++) {
        for (int f = 0; f < 8; f++) {
            uint32_t id = pci_r32(0, d, f, 0x00);
            if (id == 0xFFFFFFFFu) { if (f == 0) break; else continue; }
            uint32_t cls = pci_r32(0, d, f, 0x08);     /* [31:24]=class [23:16]=sub [15:8]=prog-if */
            if ((cls >> 24) != 0x0C || ((cls >> 16) & 0xFF) != 0x03) continue;
            uint32_t progif = (cls >> 8) & 0xFF;
            const char *k = progif == 0x30 ? "xHCI" : progif == 0x20 ? "EHCI" :
                            progif == 0x10 ? "OHCI" : progif == 0x00 ? "UHCI" : "USB?";
            kprintf("  [usb] %s controller at %02d:%d\n", k, d, f);
            if (progif == 0x30 && !bar) {
                uint32_t b0 = pci_r32(0, d, f, 0x10);
                bar = b0 & ~0xFu;                       /* 64-bit BAR: low 32 bits suffice (<4 GiB) */
                pci_w32(0, d, f, 0x04, pci_r32(0, d, f, 0x04) | 0x6); /* MEM + bus master */
            }
        }
    }
    return bar;
}

static void ring_doorbell(uint32_t slot, uint32_t target) {
    mw(dbb, slot * 4, target);
}

/* Wait for a Command Completion event on the event ring; returns completion code
 * (1 = success) or 0 on timeout. */
static uint32_t poll_event(uint32_t want_type) {
    for (int i = 0; i < 2000000; i++) {
        trb_t *e = &evt_ring[evt_deq];
        if ((e->control & 1u) == evt_cycle) {
            uint32_t type = (e->control >> 10) & 0x3F;
            uint32_t cc   = (e->status >> 24) & 0xFF;
            if (++evt_deq == EVT_TRBS) { evt_deq = 0; evt_cycle ^= 1; }
            mw(rtb, IR0 + IR_ERDP, PHYS(&evt_ring[evt_deq]) | (1u << 3)); /* advance + clear EHB */
            mw(rtb, IR0 + IR_ERDP + 4, 0);
            if (type == want_type) return cc ? cc : 1;
        }
    }
    return 0;
}

void xhci_init(void) {
    uint32_t bar = pci_find_xhci();
    if (!bar) { kprintf("  [usb] no xHCI controller found\n"); return; }

    if (!vmm_map_mmio(bar, 0x10000)) { kprintf("  [usb] could not map xHCI MMIO\n"); return; }
    capb = (uintptr_t)bar;

    uint8_t  caplen     = *(volatile uint8_t *)capb;
    uint32_t hcsparams1 = mr(capb, 0x04);
    uint32_t hcsparams2 = mr(capb, 0x08);
    uint32_t hccparams1 = mr(capb, 0x10);
    maxslots = hcsparams1 & 0xFF;
    maxports = (hcsparams1 >> 24) & 0xFF;
    opb = capb + caplen;
    rtb = capb + (mr(capb, 0x18) & ~0x1Fu);
    dbb = capb + (mr(capb, 0x14) & ~0x3u);
    kprintf("  [usb] xHCI %u ports, %u slots (HCIVERSION %x)\n",
            maxports, maxslots, (mr(capb, 0x00) >> 16) & 0xFFFF);

    take_ownership(hccparams1);

    /* Stop, then reset the controller. */
    mw(opb, OP_USBCMD, mr(opb, OP_USBCMD) & ~CMD_RS);
    if (!wait_set(opb, OP_USBSTS, STS_HCH)) { kprintf("  [usb] controller would not halt\n"); return; }
    mw(opb, OP_USBCMD, CMD_HCRST);
    if (!wait_clear(opb, OP_USBCMD, CMD_HCRST) || !wait_clear(opb, OP_USBSTS, STS_CNR)) {
        kprintf("  [usb] controller reset timed out\n"); return;
    }

    /* Slots + device context base address array. */
    uint32_t slots_en = maxslots > 63 ? 63 : maxslots;
    mw(opb, OP_CONFIG, slots_en);
    for (uint32_t i = 0; i < 64; i++) dcbaa[i] = 0;

    /* Scratchpad buffers, if the controller asks for them. */
    uint32_t nscratch = (((hcsparams2 >> 21) & 0x1F)) | (((hcsparams2 >> 27) & 0x1F) << 5);
    if (nscratch) {
        if (nscratch > MAX_SCRATCH) nscratch = MAX_SCRATCH;
        for (uint32_t i = 0; i < nscratch; i++) scratch_arr[i] = PHYS(scratch_buf[i]);
        dcbaa[0] = PHYS(scratch_arr);
        kprintf("  [usb] %u scratchpad buffer(s)\n", nscratch);
    }
    mw(opb, OP_DCBAAP, PHYS(dcbaa));
    mw(opb, OP_DCBAAP + 4, 0);

    /* Command ring: a Link TRB in the last slot points back to the start. */
    for (uint32_t i = 0; i < CMD_TRBS; i++) { cmd_ring[i].lo = cmd_ring[i].hi = cmd_ring[i].status = cmd_ring[i].control = 0; }
    cmd_ring[CMD_TRBS - 1].lo = PHYS(cmd_ring);
    cmd_ring[CMD_TRBS - 1].control = (6u << 10) | (1u << 1) | cmd_cycle; /* Link TRB, Toggle Cycle */
    cmd_enq = 0; cmd_cycle = 1;
    mw(opb, OP_CRCR, PHYS(cmd_ring) | 1u);   /* RCS = 1 */
    mw(opb, OP_CRCR + 4, 0);

    /* Event ring + segment table on interrupter 0. */
    for (uint32_t i = 0; i < EVT_TRBS; i++) { evt_ring[i].lo = evt_ring[i].hi = evt_ring[i].status = evt_ring[i].control = 0; }
    evt_deq = 0; evt_cycle = 1;
    erst[0].base = PHYS(evt_ring); erst[0].size = EVT_TRBS; erst[0].rsvd = 0;
    mw(rtb, IR0 + IR_ERSTSZ, 1);
    mw(rtb, IR0 + IR_ERDP, PHYS(evt_ring)); mw(rtb, IR0 + IR_ERDP + 4, 0);
    mw(rtb, IR0 + IR_ERSTBA, PHYS(erst));   mw(rtb, IR0 + IR_ERSTBA + 4, 0);

    /* Run. */
    mw(opb, OP_USBCMD, mr(opb, OP_USBCMD) | CMD_RS);
    if (!wait_clear(opb, OP_USBSTS, STS_HCH)) { kprintf("  [usb] controller would not start\n"); return; }

    /* Prove the rings: issue a No-Op command and wait for its completion event. */
    trb_t *t = &cmd_ring[cmd_enq];
    t->lo = t->hi = t->status = 0;
    t->control = (TRB_NOOP_CMD << 10) | cmd_cycle;
    if (++cmd_enq == CMD_TRBS - 1) { cmd_enq = 0; cmd_cycle ^= 1; }  /* wrap before the Link TRB */
    ring_doorbell(0, 0);
    uint32_t cc = poll_event(TRB_CMD_COMPLETION);
    kprintf("  [usb] No-Op command -> %s (cc=%u)\n", cc == 1 ? "completed" : "no event", cc);

    /* Report port connection status (proves the controller sees attached HID). */
    int connected = 0;
    for (uint32_t p = 1; p <= maxports; p++) {
        uint32_t sc = mr(opb, OP_PORTSC(p));
        if (sc & 1u) {
            connected++;
            kprintf("  [usb] port %u: device attached (speed id %u)\n", p, (sc >> 10) & 0xF);
        }
    }
    if (!connected) kprintf("  [usb] no devices attached to any port\n");

    present = (cc == 1);
}
