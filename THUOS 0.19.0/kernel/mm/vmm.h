/* THUOS — virtual memory (paging) glue.
 * Builds a real x86 page directory + page tables that identity-map low memory,
 * driven by the pure logic in vmm_core.c. It is STAGED: the tables are
 * constructed and inspectable, but paging is NOT enabled (CR0.PG is not set)
 * because enabling it cannot be verified without QEMU/hardware — flipping it
 * unverified risks a silent triple-fault. Enable is compiled in only when
 * THUOS_ENABLE_PAGING_EXPERIMENTAL is defined, and must be boot-verified. */
#ifndef THUOS_VMM_H
#define THUOS_VMM_H

#include "types.h"

void     vmm_init(void);            /* construct identity-map page tables */
int      vmm_is_ready(void);        /* 1 if tables were built */
uint32_t vmm_dir_phys(void);        /* physical address of the page directory */
uint32_t vmm_mapped_bytes(void);    /* bytes identity-mapped */
uint32_t vmm_phys_of(uint32_t va);  /* translate va -> pa, or 0xFFFFFFFF */
void     vmm_enable(void);           /* load CR3 + set CR0.PG (identity map) */
int      vmm_is_enabled(void);       /* 1 once paging is on */
int      vmm_map_lfb(uint32_t phys, uint32_t bytes); /* identity-map a high MMIO region (e.g. the framebuffer) */
uint32_t vmm_lfb_capacity(void);     /* max bytes vmm_map_lfb can cover */
int      vmm_map_mmio(uint32_t phys, uint32_t bytes); /* identity-map device MMIO (xHCI BAR), cache-disabled */

#endif /* THUOS_VMM_H */
