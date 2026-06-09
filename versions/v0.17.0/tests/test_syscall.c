/* THUOS — host unit test for the pure syscall dispatch core. make test */
#define THUOS_HOSTED_TEST
#include "../kernel/arch/x86/syscall_core.c"

#include <stdio.h>
#include <assert.h>

static int32_t h_add(uint32_t a, uint32_t b, uint32_t c) { (void)c; return (int32_t)(a + b); }
static int32_t h_neg(uint32_t a, uint32_t b, uint32_t c) { (void)b; (void)c; return -(int32_t)a; }

int main(void) {
    syscall_fn tbl[SYS_MAX];
    syscall_table_init(tbl, SYS_MAX);

    /* unknown numbers -> ENOSYS */
    assert(syscall_call(tbl, SYS_MAX, 0, 0, 0, 0) == SYS_ENOSYS);
    assert(syscall_call(tbl, SYS_MAX, 999, 0, 0, 0) == SYS_ENOSYS);

    /* register + route */
    assert(syscall_register(tbl, SYS_MAX, 0, h_add) == 0);
    assert(syscall_register(tbl, SYS_MAX, 5, h_neg) == 0);
    assert(syscall_register(tbl, SYS_MAX, -1, h_add) == -1);     /* bad num */
    assert(syscall_register(tbl, SYS_MAX, SYS_MAX, h_add) == -1);

    assert(syscall_call(tbl, SYS_MAX, 0, 7, 35, 0) == 42);       /* h_add */
    assert(syscall_call(tbl, SYS_MAX, 5, 9, 0, 0) == -9);        /* h_neg */
    assert(syscall_call(tbl, SYS_MAX, 1, 0, 0, 0) == SYS_ENOSYS);/* gap   */

    /* overwrite a slot */
    syscall_register(tbl, SYS_MAX, 0, h_neg);
    assert(syscall_call(tbl, SYS_MAX, 0, 4, 0, 0) == -4);

    printf("syscall dispatch test: OK\n");
    printf("  register/route, unknown->ENOSYS, bounds, overwrite\n");
    return 0;
}
