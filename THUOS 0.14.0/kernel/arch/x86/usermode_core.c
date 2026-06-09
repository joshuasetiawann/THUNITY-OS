/* THUOS — pure user-mode (ring 3) descriptor/selector math. See header. */
#include "usermode_core.h"

uint16_t um_selector(uint16_t index, uint8_t rpl) {
    return (uint16_t)((index << 3) | (rpl & 3u));
}

uint32_t um_user_eflags(uint32_t base_flags) {
    return (base_flags & ~UM_EFLAGS_NT) | UM_EFLAGS_IF | UM_EFLAGS_RSVD1;
}

void um_tss_descriptor(uint32_t base, uint32_t limit, uint8_t out[8]) {
    out[0] = (uint8_t)(limit & 0xFFu);
    out[1] = (uint8_t)((limit >> 8) & 0xFFu);
    out[2] = (uint8_t)(base & 0xFFu);
    out[3] = (uint8_t)((base >> 8) & 0xFFu);
    out[4] = (uint8_t)((base >> 16) & 0xFFu);
    out[5] = 0x89u;                                  /* P|DPL0|S=0|type=9 (32-bit TSS) */
    out[6] = (uint8_t)((limit >> 16) & 0x0Fu);       /* G=0, AVL=0, limit[19:16] */
    out[7] = (uint8_t)((base >> 24) & 0xFFu);
}
