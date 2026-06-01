/* THUOS — host unit test for the pure paging logic (vmm_core.c).
 *
 * Compiles the SAME vmm_core.c the kernel uses (THUOS_HOSTED_TEST) and checks
 * the address split, entry encode/decode, identity-map construction, address
 * translation, and an arbitrary virtual->physical mapping. No QEMU needed.
 *
 * Build & run:  make test   (or: gcc -O2 tests/test_vmm.c -o build/test_vmm) */
#define THUOS_HOSTED_TEST
#include "../kernel/mm/vmm_core.c"

#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    /* 1) virtual-address split: [10 PD | 10 PT | 12 offset] */
    assert(vmm_pde_index(0x00000000u) == 0);
    assert(vmm_pde_index(0x00400000u) == 1);     /* 4 MiB  -> PDE 1   */
    assert(vmm_pde_index(0xC0000000u) == 768);   /* 3 GiB  -> PDE 768 */
    assert(vmm_pte_index(0x00001000u) == 1);
    assert(vmm_pte_index(0x00401000u) == 1);
    assert(vmm_offset(0x00000ABCu) == 0xABC);

    /* 2) entry encode/decode */
    uint32_t e = vmm_make_entry(0x12345000u, PTE_PRESENT | PTE_RW);
    assert(vmm_entry_addr(e) == 0x12345000u);
    assert(vmm_entry_present(e));
    assert(e & PTE_RW);
    assert(!vmm_entry_present(vmm_make_entry(0x1000u, 0)));

    /* 3) identity-map the low 8 MiB across a 2-table pool */
    enum { NPTS = 2 };
    static uint32_t pd[1024];
    static uint32_t pts[NPTS * 1024];
    uint32_t pts_phys = 0x00100000u;             /* synthetic; pool is contiguous */
    uint32_t n = vmm_identity_map(pd, pts, pts_phys, NPTS, 8u * 1024 * 1024, PTE_RW);
    assert(n == 2);

    /* 4) inside the mapped range, translate(va) == va */
    uint32_t phys;
    uint32_t samples[] = {0x0u, 0x1000u, 0xABCDu, 0x00400000u, 0x007FF000u, 0x007FFFFFu};
    for (unsigned i = 0; i < sizeof samples / sizeof samples[0]; i++) {
        assert(vmm_translate(pd, pts, pts_phys, samples[i], &phys) == 1);
        assert(phys == samples[i]);
    }
    /* 5) just past 8 MiB -> not mapped */
    assert(vmm_translate(pd, pts, pts_phys, 0x00800000u, &phys) == 0);

    /* 6) arbitrary mapping: va 0xC0000000 -> pa 0x00200000 via table 1 */
    uint32_t *pt1 = pts + 1024;
    uint32_t  pt1_phys = pts_phys + 4096;
    memset(pt1, 0, 1024 * sizeof(uint32_t));
    vmm_set_pde(pd, 0xC0000000u, pt1_phys, PTE_PRESENT | PTE_RW);
    vmm_set_pte(pt1, 0xC0000000u, 0x00200000u, PTE_PRESENT | PTE_RW);
    assert(vmm_translate(pd, pts, pts_phys, 0xC0000123u, &phys) == 1);
    assert(phys == 0x00200123u);                 /* offset preserved */

    printf("vmm (paging) test: OK\n");
    printf("  addr split, entry encode/decode, identity map,\n");
    printf("  translate==identity, unmapped->0, arbitrary va->pa\n");
    return 0;
}
