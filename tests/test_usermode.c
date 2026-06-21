/* THUOS — host unit test for the pure user-mode descriptor/selector core.
 * make test */
#define THUOS_HOSTED_TEST
#include "../kernel/arch/x86/usermode_core.c"

#include <stdio.h>
#include <assert.h>

int main(void) {
    /* selector composition: index<<3 | rpl, and the literals the asm uses */
    assert(um_selector(UM_KCODE_IDX, 0) == UM_KCODE_SEL);   /* 0x08 */
    assert(um_selector(UM_KDATA_IDX, 0) == UM_KDATA_SEL);   /* 0x10 */
    assert(um_selector(UM_UCODE_IDX, 3) == UM_UCODE_SEL);   /* 0x1B */
    assert(um_selector(UM_UDATA_IDX, 3) == UM_UDATA_SEL);   /* 0x23 */
    assert(um_selector(UM_TSS_IDX,   0) == UM_TSS_SEL);     /* 0x28 */
    assert(um_selector(2, 3) == 0x13u);                     /* rpl bits land */

    /* ring-3 EFLAGS: IF on, reserved bit on, NT cleared, other bits preserved */
    assert((um_user_eflags(0) & UM_EFLAGS_IF) == UM_EFLAGS_IF);
    assert(um_user_eflags(0) == (UM_EFLAGS_IF | UM_EFLAGS_RSVD1));
    assert((um_user_eflags(UM_EFLAGS_NT) & UM_EFLAGS_NT) == 0);  /* NT stripped */
    assert((um_user_eflags(0x44u) & 0x44u) == 0x44u);            /* unrelated bits kept */

    /* TSS descriptor encoding (the 8 bytes the CPU reads from the GDT) */
    uint8_t d[8];
    um_tss_descriptor(0x00123456u, 0x00000067u, d);
    assert(d[0] == 0x67 && d[1] == 0x00);                   /* limit[15:0]  */
    assert(d[2] == 0x56 && d[3] == 0x34 && d[4] == 0x12);   /* base[23:0]   */
    assert(d[7] == 0x00);                                   /* base[31:24]  */
    assert(d[5] == 0x89);                                   /* present 32-bit TSS, DPL 0 */
    assert((d[6] & 0x0Fu) == 0x00);                         /* limit[19:16] */
    assert((d[6] & 0x80u) == 0x00);                         /* G = 0 (byte granular) */

    /* a larger limit spills its high nibble into the granularity byte */
    um_tss_descriptor(0xCAFEBABEu, 0x000FFFFFu, d);
    assert(d[0] == 0xFF && d[1] == 0xFF);
    assert((d[6] & 0x0Fu) == 0x0Fu);
    assert(d[2] == 0xBE && d[3] == 0xBA && d[4] == 0xFE && d[7] == 0xCA);

    printf("usermode core test: OK\n");
    printf("  selectors (kcode/kdata/ucode/udata/tss), ring-3 EFLAGS (IF/NT), TSS descriptor bytes\n");
    return 0;
}
