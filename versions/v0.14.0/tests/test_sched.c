/* THUOS — host unit test for the pure scheduler policy (sched_core.c).
 *
 * Compiles the SAME sched_core.c the kernel uses (THUOS_HOSTED_TEST) and checks
 * round-robin fairness, block/unblock, time-slice expiry, and exit. No QEMU.
 *
 * Build & run:  make test   (or: gcc -O2 tests/test_sched.c -o build/test_sched) */
#define THUOS_HOSTED_TEST
#include "../kernel/sched/sched_core.c"

#include <stdio.h>
#include <assert.h>

int main(void) {
    sched_task_t storage[4];
    sched_t s;

    /* 1) empty scheduler */
    sched_init(&s, storage, 4);
    assert(sched_current(&s) == -1);
    assert(sched_runnable(&s) == 0);
    assert(sched_pick_next(&s) == -1);

    /* 2) three tasks rotate fairly: 1,2,3,1,2,3 */
    assert(sched_add(&s, 1, 3) >= 0);
    assert(sched_add(&s, 2, 3) >= 0);
    assert(sched_add(&s, 3, 3) >= 0);
    assert(sched_runnable(&s) == 3);
    int order[6];
    for (int i = 0; i < 6; i++) order[i] = sched_pick_next(&s);
    assert(order[0] == 1 && order[1] == 2 && order[2] == 3);
    assert(order[3] == 1 && order[4] == 2 && order[5] == 3);

    /* 3) blocking a task removes it from rotation; unblocking restores it */
    sched_block(&s, 2);
    assert(sched_runnable(&s) == 2);
    for (int i = 0; i < 4; i++) assert(sched_pick_next(&s) != 2);
    sched_unblock(&s, 2);
    int seen2 = 0;
    for (int i = 0; i < 3; i++) if (sched_pick_next(&s) == 2) seen2 = 1;
    assert(seen2);

    /* 4) time slice: pick reloads quantum=3; it expires on the 3rd tick */
    sched_init(&s, storage, 4);
    sched_add(&s, 7, 3);
    assert(sched_pick_next(&s) == 7);
    assert(sched_on_tick(&s) == 0);
    assert(sched_on_tick(&s) == 0);
    assert(sched_on_tick(&s) == 1);

    /* 5) exit removes a task; only the survivor is ever picked */
    sched_init(&s, storage, 4);
    sched_add(&s, 1, 2);
    sched_add(&s, 2, 2);
    sched_pick_next(&s);
    sched_exit(&s, 1);
    assert(sched_runnable(&s) == 1);
    assert(sched_pick_next(&s) == 2);
    assert(sched_pick_next(&s) == 2);

    printf("scheduler test: OK\n");
    printf("  round-robin fairness, block/unblock, quantum expiry, exit\n");
    return 0;
}
