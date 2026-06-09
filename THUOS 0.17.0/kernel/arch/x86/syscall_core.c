/* THUOS — pure syscall dispatch table. No kernel dependencies. */
#include "syscall_core.h"

void syscall_table_init(syscall_fn *tbl, int max) {
    for (int i = 0; i < max; i++) tbl[i] = 0;
}

int syscall_register(syscall_fn *tbl, int max, int num, syscall_fn fn) {
    if (num < 0 || num >= max) return -1;
    tbl[num] = fn;
    return 0;
}

int32_t syscall_call(const syscall_fn *tbl, int max, uint32_t num,
                     uint32_t a, uint32_t b, uint32_t c) {
    if (num >= (uint32_t)max || !tbl[num]) return SYS_ENOSYS;
    return tbl[num](a, b, c);
}
