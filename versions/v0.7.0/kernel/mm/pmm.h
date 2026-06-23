/* THUOS — physical memory manager (4 KiB page-frame allocator). */
#ifndef THUOS_PMM_H
#define THUOS_PMM_H

#include "types.h"

#define PMM_PAGE_SIZE 4096u

/* Parse the Multiboot memory map, build the frame bitmap, and reserve the
 * protected regions (low 1 MiB, kernel image, Multiboot structures). */
void pmm_init(uint32_t magic, uint32_t mb_info_addr);

/* Allocate one physical 4 KiB frame. Returns its physical address, or 0 on
 * failure (frame 0 is always reserved, so 0 is an unambiguous "no memory"). */
uint32_t pmm_alloc_frame(void);

/* Free a previously allocated frame. Returns 0 on success, or a negative code:
 * -1 unaligned/out-of-range, -2 protected region, -3 was already free. */
int pmm_free_frame(uint32_t phys_addr);

/* Statistics (frame counts and bytes). */
uint32_t pmm_total_frames(void);     /* frames spanned by usable RAM (capped) */
uint32_t pmm_usable_frames(void);    /* frames in AVAILABLE regions           */
uint32_t pmm_reserved_frames(void);  /* usable frames held by protected ranges*/
uint32_t pmm_free_frames(void);      /* currently free frames                  */
uint32_t pmm_used_frames(void);      /* reserved + allocated                   */
uint32_t pmm_usable_bytes(void);     /* total AVAILABLE bytes (low 32 bits)    */
uint32_t pmm_mem_lower_kb(void);
uint32_t pmm_mem_upper_kb(void);

int  pmm_is_protected(uint32_t phys_addr);
int  pmm_available(void);            /* 1 if a memory map was parsed */
void pmm_print_mmap(void);           /* dump the Multiboot memory map */
void pmm_print_stats(void);          /* dump frame statistics */

#endif /* THUOS_PMM_H */
