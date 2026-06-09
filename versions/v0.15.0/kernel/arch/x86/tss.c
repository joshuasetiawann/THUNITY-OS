/* THUOS — Task State Segment. See tss.h for why a TSS is required for ring 3. */
#include "tss.h"
#include "usermode_core.h"   /* UM_KDATA_SEL, UM_TSS_IDX, um_selector */

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0; uint32_t ss0;             /* the only fields the CPU reads here */
    uint32_t esp1; uint32_t ss1;
    uint32_t esp2; uint32_t ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

static struct tss_entry g_tss;

/* The stack the CPU switches to when it enters ring 0 from ring 3. It must be
 * separate from the boot stack (so a ring-3 syscall/IRQ cannot corrupt the
 * kernel context that issued the user-mode entry). */
static uint8_t  tss_kstack[8192] __attribute__((aligned(16)));

void tss_init(void) {
    uint8_t *p = (uint8_t *)&g_tss;
    for (uint32_t i = 0; i < sizeof g_tss; i++) p[i] = 0;
    g_tss.ss0        = UM_KDATA_SEL;                                   /* 0x10 */
    g_tss.esp0       = (uint32_t)(uintptr_t)(tss_kstack + sizeof tss_kstack);
    g_tss.iomap_base = (uint16_t)sizeof g_tss;   /* past the limit => no I/O bitmap */
}

void     tss_set_esp0(uint32_t esp0) { g_tss.esp0 = esp0; }
uint32_t tss_base(void)              { return (uint32_t)(uintptr_t)&g_tss; }
uint32_t tss_limit(void)             { return (uint32_t)sizeof g_tss - 1u; }
uint16_t tss_selector(void)          { return um_selector(UM_TSS_IDX, 0); }   /* 0x28 */
