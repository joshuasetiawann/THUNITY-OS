/* THUOS — `features` command. Builds the honest capability map. See header. */
#include "types.h"
#include "kprintf.h"
#include "feature_registry.h"
#include "osreport.h"

static feature_registry_t g_reg;   /* static: keep it off the kernel stack */

static void build(void) {
    feat_init(&g_reg);
    /* implemented + boot-verified in QEMU */
    feat_add(&g_reg, "Multiboot boot + 1 MiB load",          FEAT_IMPLEMENTED);
    feat_add(&g_reg, "VGA text + COM1 serial + kprintf",     FEAT_IMPLEMENTED);
    feat_add(&g_reg, "GDT/IDT/exceptions/PIC/IRQ/PIT",       FEAT_IMPLEMENTED);
    feat_add(&g_reg, "PS/2 keyboard + mouse",                FEAT_IMPLEMENTED);
    feat_add(&g_reg, "USB xHCI + USB-HID keyboard/mouse",    FEAT_IMPLEMENTED);
    feat_add(&g_reg, "Paging enabled (CR0.PG)",              FEAT_IMPLEMENTED);
    feat_add(&g_reg, "Cooperative multitasking",             FEAT_IMPLEMENTED);
    feat_add(&g_reg, "In-RAM filesystem (ls/cat/write)",     FEAT_IMPLEMENTED);
    feat_add(&g_reg, "int 0x80 syscalls + ring-3 user mode", FEAT_IMPLEMENTED);
    feat_add(&g_reg, "1024x768 desktop + apps",              FEAT_IMPLEMENTED);
    feat_add(&g_reg, "Interactive shell (thuos>)",           FEAT_IMPLEMENTED);
    feat_add(&g_reg, "Kernel log + dmesg",                   FEAT_IMPLEMENTED);
    feat_add(&g_reg, "AI shell namespace (ai ...)",          FEAT_IMPLEMENTED);
    /* host-tested logic (make test) */
    feat_add(&g_reg, "PMM / heap / paging core",             FEAT_HOST_TESTED);
    feat_add(&g_reg, "Scheduler / fs / syscall / usermode",  FEAT_HOST_TESTED);
    feat_add(&g_reg, "AI core: registry/policy/task/audit",  FEAT_HOST_TESTED);
    feat_add(&g_reg, "OS feature/status registry",           FEAT_HOST_TESTED);
    /* designed, but NO runtime code path yet — the honest frontier */
    feat_add(&g_reg, "Thunity AI bridge (to local server)",  FEAT_DESIGN_ONLY);
    feat_add(&g_reg, "AI model runtime / inference",         FEAT_DESIGN_ONLY);
    feat_add(&g_reg, "TCP/IP networking",                    FEAT_DESIGN_ONLY);
    feat_add(&g_reg, "Storage / disk driver",                FEAT_DESIGN_ONLY);
    feat_add(&g_reg, "ELF loader (apps from files)",         FEAT_DESIGN_ONLY);
    feat_add(&g_reg, "Window manager (movable windows)",     FEAT_DESIGN_ONLY);
    feat_add(&g_reg, "Multi-user accounts / sessions",       FEAT_DESIGN_ONLY);
    feat_add(&g_reg, "Package manager (real install)",       FEAT_DESIGN_ONLY);
    feat_add(&g_reg, "UEFI boot path (grub-efi)",            FEAT_DESIGN_ONLY);
    /* exists but not verifiable in this environment */
    feat_add(&g_reg, "Boot on real physical hardware",       FEAT_NOT_VERIFIED);
}

void osreport_print(void) {
    build();
    kprintf("THUOS feature map — honest status (one source of truth)\n");
    kprintf("  implemented=%d host-tested=%d compile-only=%d design-only=%d not-verified=%d\n",
            feat_count_by(&g_reg, FEAT_IMPLEMENTED), feat_count_by(&g_reg, FEAT_HOST_TESTED),
            feat_count_by(&g_reg, FEAT_COMPILE_ONLY), feat_count_by(&g_reg, FEAT_DESIGN_ONLY),
            feat_count_by(&g_reg, FEAT_NOT_VERIFIED));
    for (int st = 0; st < FEAT_STATUS_COUNT; st++) {
        if (feat_count_by(&g_reg, (feat_status_t)st) == 0) continue;
        kprintf("[%s]\n", feat_status_label((feat_status_t)st));
        for (int i = 0; i < feat_count(&g_reg); i++) {
            const feature_t *f = feat_get(&g_reg, i);
            if (f && f->status == (feat_status_t)st) kprintf("  - %s\n", f->name);
        }
    }
    kprintf("Legend: implemented=boot-verified  host-tested=make test  design-only=no runtime path yet\n");
}
