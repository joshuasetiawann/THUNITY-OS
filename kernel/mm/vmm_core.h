/* THUOS — pure x86 (i386) paging logic. No kernel/hardware dependency, so the
 * exact page-table construction and address-translation code the kernel relies
 * on can be unit-tested on the host (see tests/test_vmm.c).
 *
 * 32-bit 2-level paging: a virtual address splits as
 *   [ 10 bits PD index | 10 bits PT index | 12 bits offset ]
 * A page directory has 1024 PDEs; each page table has 1024 PTEs; pages are 4 KiB.
 * Entries hold a 4 KiB-aligned physical address in the top 20 bits plus flags
 * in the low 12 bits. */
#ifndef THUOS_VMM_CORE_H
#define THUOS_VMM_CORE_H

#ifdef THUOS_HOSTED_TEST
#include <stdint.h>
#include <stddef.h>
#else
#include "types.h"
#endif

/* PDE/PTE flag bits (Intel SDM). */
#define PTE_PRESENT   0x001u
#define PTE_RW        0x002u
#define PTE_USER      0x004u
#define PTE_PWT       0x008u
#define PTE_PCD       0x010u
#define PTE_ACCESSED  0x020u
#define PTE_DIRTY     0x040u

#define VMM_PAGE_SIZE 4096u

/* Address splitting. */
uint32_t vmm_pde_index(uint32_t va);   /* va >> 22            */
uint32_t vmm_pte_index(uint32_t va);   /* (va >> 12) & 0x3FF  */
uint32_t vmm_offset(uint32_t va);      /* va & 0xFFF          */

/* Entry encode/decode. */
uint32_t vmm_make_entry(uint32_t phys, uint32_t flags); /* (phys&~0xFFF)|(flags&0xFFF) */
uint32_t vmm_entry_addr(uint32_t entry);                /* entry & ~0xFFF */
int      vmm_entry_present(uint32_t entry);             /* entry & PTE_PRESENT */

/* Single-entry setters (caller includes PTE_PRESENT in flags). */
void vmm_set_pde(uint32_t *pd, uint32_t va, uint32_t pt_phys, uint32_t flags);
void vmm_set_pte(uint32_t *pt, uint32_t va, uint32_t phys,    uint32_t flags);

/* Identity-map [0, bytes) into pd using a contiguous pool of n_pts page tables
 * that begin at physical address pts_phys (pts is the matching pointer). The pd
 * is zeroed first. Returns the number of page tables used, or 0 if the pool is
 * too small. */
uint32_t vmm_identity_map(uint32_t *pd, uint32_t *pts, uint32_t pts_phys,
                          uint32_t n_pts, uint32_t bytes, uint32_t flags);

/* Translate va through pd (page tables live in the contiguous pool at pts_phys).
 * Returns 1 and writes *phys if mapped+present, else 0. */
int vmm_translate(const uint32_t *pd, const uint32_t *pts, uint32_t pts_phys,
                  uint32_t va, uint32_t *phys);

#endif /* THUOS_VMM_CORE_H */
