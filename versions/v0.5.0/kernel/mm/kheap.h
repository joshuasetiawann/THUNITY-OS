/* THUOS — kernel heap (kmalloc/kfree).
 * A fixed-arena allocator (Stage-A, pre-paging): kmalloc is backed by a
 * static arena in BSS. Once paging is enabled this arena can be replaced by
 * PMM-backed, growable pages without changing this API. */
#ifndef THUOS_KHEAP_H
#define THUOS_KHEAP_H

#include "types.h"

void   kheap_init(void);
void  *kmalloc(size_t size);
void  *kcalloc(size_t nmemb, size_t size);
void  *krealloc(void *ptr, size_t size);
void   kfree(void *ptr);

size_t kheap_total(void);
size_t kheap_used(void);
size_t kheap_free(void);
size_t kheap_blocks(void);
int    kheap_check(void);

#endif /* THUOS_KHEAP_H */
