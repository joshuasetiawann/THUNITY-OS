/* THUOS — pure x86 (i386) paging logic. No kernel dependencies. */
#include "vmm_core.h"

#define ADDR_MASK 0xFFFFF000u
#define FLAG_MASK 0x00000FFFu

uint32_t vmm_pde_index(uint32_t va) { return va >> 22; }
uint32_t vmm_pte_index(uint32_t va) { return (va >> 12) & 0x3FFu; }
uint32_t vmm_offset(uint32_t va)    { return va & 0xFFFu; }

uint32_t vmm_make_entry(uint32_t phys, uint32_t flags) {
    return (phys & ADDR_MASK) | (flags & FLAG_MASK);
}
uint32_t vmm_entry_addr(uint32_t entry) { return entry & ADDR_MASK; }
int      vmm_entry_present(uint32_t entry) { return (entry & PTE_PRESENT) ? 1 : 0; }

void vmm_set_pde(uint32_t *pd, uint32_t va, uint32_t pt_phys, uint32_t flags) {
    pd[vmm_pde_index(va)] = vmm_make_entry(pt_phys, flags);
}
void vmm_set_pte(uint32_t *pt, uint32_t va, uint32_t phys, uint32_t flags) {
    pt[vmm_pte_index(va)] = vmm_make_entry(phys, flags);
}

uint32_t vmm_identity_map(uint32_t *pd, uint32_t *pts, uint32_t pts_phys,
                          uint32_t n_pts, uint32_t bytes, uint32_t flags) {
    uint32_t pages    = (bytes + VMM_PAGE_SIZE - 1) >> 12;
    uint32_t need_pts = (pages + 1023u) >> 10;        /* 1024 pages per table */
    if (need_pts > n_pts) return 0;

    for (uint32_t i = 0; i < 1024u; i++) pd[i] = 0;   /* clear directory */

    for (uint32_t t = 0; t < need_pts; t++) {
        uint32_t *pt      = pts + t * 1024u;
        uint32_t  pt_phys = pts_phys + t * VMM_PAGE_SIZE;
        pd[t] = vmm_make_entry(pt_phys, flags | PTE_PRESENT);
        for (uint32_t e = 0; e < 1024u; e++) {
            uint32_t page = t * 1024u + e;
            if (page < pages) pt[e] = vmm_make_entry(page << 12, flags | PTE_PRESENT);
            else              pt[e] = 0;
        }
    }
    return need_pts;
}

int vmm_translate(const uint32_t *pd, const uint32_t *pts, uint32_t pts_phys,
                  uint32_t va, uint32_t *phys) {
    uint32_t pde = pd[vmm_pde_index(va)];
    if (!(pde & PTE_PRESENT)) return 0;

    uint32_t pt_phys = vmm_entry_addr(pde);
    const uint32_t *pt = pts + (pt_phys - pts_phys) / 4u;   /* pool is contiguous */

    uint32_t pte = pt[vmm_pte_index(va)];
    if (!(pte & PTE_PRESENT)) return 0;

    if (phys) *phys = vmm_entry_addr(pte) | vmm_offset(va);
    return 1;
}
