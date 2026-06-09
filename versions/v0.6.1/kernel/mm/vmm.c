/* THUOS — virtual memory (paging) glue. See vmm.h for the honesty note. */
#include "vmm.h"
#include "vmm_core.h"

#define VMM_IDMAP_BYTES (8u * 1024u * 1024u)   /* identity-map the low 8 MiB */
#define VMM_NPTS        2                       /* 2 tables * 4 MiB = 8 MiB   */

/* Page directory and page tables must be 4 KiB aligned. */
static uint32_t page_dir[1024]            __attribute__((aligned(4096)));
static uint32_t page_tabs[VMM_NPTS][1024] __attribute__((aligned(4096)));
static int      ready = 0;

void vmm_init(void) {
    uint32_t pts_phys = (uint32_t)(uintptr_t)page_tabs;
    uint32_t n = vmm_identity_map(page_dir, (uint32_t *)page_tabs, pts_phys,
                                  VMM_NPTS, VMM_IDMAP_BYTES, PTE_RW);
    ready = (n == VMM_NPTS);
}

int      vmm_is_ready(void)     { return ready; }
uint32_t vmm_dir_phys(void)     { return (uint32_t)(uintptr_t)page_dir; }
uint32_t vmm_mapped_bytes(void) { return ready ? VMM_IDMAP_BYTES : 0; }

uint32_t vmm_phys_of(uint32_t va) {
    uint32_t pts_phys = (uint32_t)(uintptr_t)page_tabs;
    uint32_t phys;
    if (vmm_translate(page_dir, (uint32_t *)page_tabs, pts_phys, va, &phys))
        return phys;
    return 0xFFFFFFFFu;
}

/* EXPERIMENTAL — not enabled by default and NOT boot-verified in this dev
 * environment. Loading CR3 and setting CR0.PG against an unverified mapping can
 * triple-fault. Compile in deliberately, then verify the boot in QEMU. */
#ifdef THUOS_ENABLE_PAGING_EXPERIMENTAL
void vmm_enable(void) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(page_dir) : "memory");
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;                 /* CR0.PG */
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}
#endif
