/* THUOS — scheduler glue. See sched.h for the honesty note. */
#include "sched.h"
#include "sched_core.h"
#include "kprintf.h"

#define NTASK 8

static sched_task_t storage[NTASK];
static sched_t      sched;

void sched_kinit(void) {
    sched_init(&sched, storage, NTASK);
    /* id 0 = the kernel/shell task. The others are scheduling descriptors used
     * to demonstrate the live round-robin policy. Real per-task execution
     * arrives with the context switch (staged, needs paging + boot). */
    sched_add(&sched, 0, 5);
    sched_add(&sched, 1, 3);
    sched_add(&sched, 2, 2);
}

int sched_kcurrent(void)  { return sched_current(&sched); }
int sched_krunnable(void) { return sched_runnable(&sched); }

void sched_klist(void) {
    kprintf("Scheduler (round-robin policy core; context switch staged):\n");
    for (int i = 0; i < NTASK; i++) {
        if (storage[i].state == TASK_UNUSED) continue;
        kprintf("  task %d  state %s  quantum %u/%u  ran %u\n",
                storage[i].id, sched_state_name(storage[i].state),
                storage[i].quantum, storage[i].reload, storage[i].ran);
    }
    kprintf("  runnable %d, current %d\n",
            sched_runnable(&sched), sched_current(&sched));
}

void sched_kdemo(void) {
    kprintf("Live round-robin pick order (real policy): ");
    for (int i = 0; i < 10; i++) {
        int id = sched_pick_next(&sched);
        if (id < 0) { kprintf("[idle]"); break; }
        kprintf("%d ", id);
        while (sched_on_tick(&sched) == 0) { }   /* let the slice elapse */
    }
    kprintf("\n(this is the host-tested scheduler running in-kernel; no CPU\n");
    kprintf("context switch yet - that is staged until paging is boot-verified)\n");
}
