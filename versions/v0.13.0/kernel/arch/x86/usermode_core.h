/* THUOS — pure user-mode (ring 3) descriptor/selector math. No kernel or
 * hardware dependency, so the exact selector composition, ring-3 EFLAGS, and
 * TSS-descriptor encoding the kernel relies on are unit-tested on the host
 * (tests/test_usermode.c). The hardware glue (TSS load, iret to CPL 3) lives in
 * tss.c / usermode.c / usermode.S. */
#ifndef THUOS_USERMODE_CORE_H
#define THUOS_USERMODE_CORE_H

#ifdef THUOS_HOSTED_TEST
#include <stdint.h>
#include <stddef.h>
#else
#include "types.h"
#endif

/* GDT layout (see gdt.c):
 *   0 null · 1 kernel code · 2 kernel data · 3 user code · 4 user data · 5 TSS */
#define UM_KCODE_IDX 1u
#define UM_KDATA_IDX 2u
#define UM_UCODE_IDX 3u
#define UM_UDATA_IDX 4u
#define UM_TSS_IDX   5u

/* The selectors those indices produce. The assembly in usermode.S uses these
 * literals directly; the host test pins them so an index change can't silently
 * desync the C and asm sides. */
#define UM_KCODE_SEL 0x08u           /* (1<<3)|0 */
#define UM_KDATA_SEL 0x10u           /* (2<<3)|0 */
#define UM_UCODE_SEL 0x1Bu           /* (3<<3)|3 */
#define UM_UDATA_SEL 0x23u           /* (4<<3)|3 */
#define UM_TSS_SEL   0x28u           /* (5<<3)|0 */

/* EFLAGS bits we care about when entering ring 3. */
#define UM_EFLAGS_IF    0x00000200u  /* interrupts enabled        */
#define UM_EFLAGS_RSVD1 0x00000002u  /* always-1 reserved bit     */
#define UM_EFLAGS_NT    0x00004000u  /* nested task (must be 0 for iret) */

/* selector = (index << 3) | (rpl & 3); table indicator 0 (GDT). */
uint16_t um_selector(uint16_t index, uint8_t rpl);

/* Ring-3 EFLAGS derived from a base value: IF on, reserved bit on, NT cleared.
 * IOPL is left as-is (0 in kernel context), so ring 3 cannot do port I/O. */
uint32_t um_user_eflags(uint32_t base_flags);

/* Encode the 8 bytes of an available 32-bit TSS descriptor (what the CPU reads
 * from the GDT): access = 0x89 (P=1, DPL=0, S=0, type=9), granularity byte =
 * G=0 with limit[19:16]. */
void um_tss_descriptor(uint32_t base, uint32_t limit, uint8_t out[8]);

#endif /* THUOS_USERMODE_CORE_H */
