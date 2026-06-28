/* THUOS — pure syscall dispatch table. No kernel deps, so the routing logic is
 * host-tested (tests/test_syscall.c). The real handlers + the int 0x80 entry
 * live in syscall.c. */
#ifndef THUOS_SYSCALL_CORE_H
#define THUOS_SYSCALL_CORE_H

#ifdef THUOS_HOSTED_TEST
#include <stdint.h>
#include <stddef.h>
#else
#include "types.h"
#endif

#define SYS_ENOSYS (-38)     /* unknown syscall number */
#define SYS_MAX    16

typedef int32_t (*syscall_fn)(uint32_t a, uint32_t b, uint32_t c);

void    syscall_table_init(syscall_fn *tbl, int max);
int     syscall_register(syscall_fn *tbl, int max, int num, syscall_fn fn); /* 0 ok, -1 bad num */
int32_t syscall_call(const syscall_fn *tbl, int max, uint32_t num,
                     uint32_t a, uint32_t b, uint32_t c);                    /* SYS_ENOSYS if missing */

#endif /* THUOS_SYSCALL_CORE_H */
