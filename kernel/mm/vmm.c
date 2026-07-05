/* THUOS — virtual memory (paging) glue. See vmm.h for the honesty note. */
#include "vmm.h"
#include "vmm_core.h"

#define VMM_IDMAP_BYTES (8u * 1024u * 1024u)   /* identity-map the low 8 MiB */
#define VMM_NPTS        2                       /* 2 tables * 4 MiB = 8 MiB   */

/* Page directory and page tables must be 4 KiB aligned. */
static uint32_t page_dir[1024]            __attribute__((aligned(4096)));
static uint32_t page_tabs[VMM_NPTS][1024] __attribute__((aligned(4096)));
/* Extra page tables to map a high-memory MMIO window (the linear framebuffer,
 * which a PCI BAR / GOP places far above the identity-mapped low RAM). 8 tables
 * = 32 MiB, enough for the framebuffer of any panel up to ~2560x1600x32. */
#define VMM_LFB_NPTS 8                     /* up to 32 MiB */
static uint32_t lfb_tabs[VMM_LFB_NPTS][1024] __attribute__((aligned(4096)));
static int      ready = 0;
static int      enabled = 0;

void vmm_init(void) {
    uint32_t pts_phys = (uint32_t)(uintptr_t)page_tabs;
    /* PTE_USER makes the low 8 MiB reachable from ring 3, which is what lets the
     * user-mode self-test (0.12) execute and use a stack at CPL 3. This is a
     * single flat map shared by kernel and user — NOT per-process isolation
     * (a later milestone); ring 0 is unaffected (it can always touch user pages). */
    uint32_t n = vmm_identity_map(page_dir, (uint32_t *)page_tabs, pts_phys,
                                  VMM_NPTS, VMM_IDMAP_BYTES, PTE_RW | PTE_USER);
    ready = (n == VMM_NPTS);
}

int      vmm_is_ready(void)     { return ready; }
uint32_t vmm_dir_phys(void)     { return (uint32_t)(uintptr_t)page_dir; }
uint32_t vmm_mapped_bytes(void) { return ready ? VMM_IDMAP_BYTES : 0; }

/* Largest framebuffer span vmm_map_lfb() can cover. The LFB driver clamps to
 * this so it never draws into an unmapped page. */
uint32_t vmm_lfb_capacity(void) { return VMM_LFB_NPTS * 4u * 1024u * 1024u; }

uint32_t vmm_phys_of(uint32_t va) {
    uint32_t pts_phys = (uint32_t)(uintptr_t)page_tabs;
    uint32_t phys;
    if (vmm_translate(page_dir, (uint32_t *)page_tabs, pts_phys, va, &phys))
        return phys;
    return 0xFFFFFFFFu;
}

/* Enable paging: load CR3 with the page directory and set CR0.PG. The identity
 * map covers the low 8 MiB, which contains the whole kernel image, stack, BSS
 * (heap arena + PMM bitmap + these page tables) and VGA memory, so execution
 * continues seamlessly (virtual == physical). Boot-verified in QEMU by the CI
 * boot-smoke job: if this faulted, the kernel would never reach `thuos>`. */
int vmm_is_enabled(void) { return enabled; }

void vmm_enable(void) {
    if (!ready) return;                 /* never enable an unbuilt mapping */
    uint32_t pd = (uint32_t)(uintptr_t)page_dir;
    __asm__ volatile("mov %0, %%cr3" :: "r"(pd) : "memory");
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;                 /* CR0.PG */
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
    enabled = 1;
}

/* Identity-map [phys, phys+bytes) using dedicated 4 MiB-aligned page tables.
 * The linear framebuffer sits at a PCI BAR far above the low identity map, so it
 * needs its own PDEs/PTEs before the kernel can draw to it. Supervisor + R/W. */
int vmm_map_lfb(uint32_t phys, uint32_t bytes) {
    if (!ready) return 0;
    const uint32_t FOURMB = 4u * 1024u * 1024u;
    uint32_t base = phys & ~(FOURMB - 1);              /* 4 MiB align down */
    uint32_t npts = (phys + bytes - base + FOURMB - 1) / FOURMB;
    if (npts == 0 || npts > VMM_LFB_NPTS) {
        if (npts == 0) return 0;
        npts = VMM_LFB_NPTS;
    }
    for (uint32_t t = 0; t < npts; t++) {
        uint32_t va  = base + t * FOURMB;
        uint32_t *pt = lfb_tabs[t];
        page_dir[va >> 22] = vmm_make_entry((uint32_t)(uintptr_t)pt, PTE_RW | PTE_PRESENT);
        for (uint32_t e = 0; e < 1024u; e++)
            pt[e] = vmm_make_entry(va + e * VMM_PAGE_SIZE, PTE_RW | PTE_PRESENT);
    }
    uint32_t pd = (uint32_t)(uintptr_t)page_dir;       /* reload CR3 -> flush TLB */
    __asm__ volatile("mov %0, %%cr3" :: "r"(pd) : "memory");
    return 1;
}
