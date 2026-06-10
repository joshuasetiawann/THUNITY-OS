/* THUOS — cooperative multitasking.
 *
 * Several kernel tasks, each on its own stack, run concurrently and hand the CPU
 * to one another by calling coop_yield(), which asks the round-robin scheduler
 * (sched_core) who runs next and performs a real context switch (context.S) to
 * it. When a task finishes it marks itself exited and yields; once no task is
 * runnable, control returns to the kernel main/shell context. This combines the
 * separately host-tested scheduler policy with the boot-verified context switch
 * into actual multitasking. */
#include "coop.h"
#include "sched_core.h"
#include "task.h"
#include "kprintf.h"

#define NCOOP   3
#define CSTACK  4096
#define ITERS   3

static sched_task_t cstore[NCOOP + 1];
static sched_t      csched;
static ktask_t      ctask[NCOOP];
static uint8_t      cstack[NCOOP][CSTACK] __attribute__((aligned(16)));
static ktask_t      cmain;                 /* the context that started the demo */
static int          current;               /* running task index, or -1 = main  */

/* Hand off to whoever the scheduler picks next (or back to main if none). */
static void coop_yield(void) {
    int prev = current;
    int nid  = sched_pick_next(&csched);   /* task id == index, or -1 */
    int next = (nid < 0) ? -1 : nid;
    if (next == prev) return;              /* keep running current (incl. main idle) */

    current = next;
    uint32_t *save = (prev < 0) ? &cmain.esp : &ctask[prev].esp;
    uint32_t  nsp  = (next < 0) ?  cmain.esp :  ctask[next].esp;
    thuos_context_switch(save, nsp);
}

/* Shared task body; each task runs this on its own stack with its own `me`. */
static void coop_worker(void) {
    int me = current;                      /* set by coop_yield just before switch-in */
    for (int i = 0; i < ITERS; i++) {
        kprintf("  [task %d] tick %d (own stack, via scheduler)\n", me, i + 1);
        coop_yield();
    }
    kprintf("  [task %d] done\n", me);
    sched_exit(&csched, me);
    coop_yield();                          /* -> next task, or back to main */
    for (;;) { }                           /* unreachable */
}

void coop_run_demo(void) {
    sched_init(&csched, cstore, NCOOP + 1);
    current = -1;
    for (int i = 0; i < NCOOP; i++) {
        sched_add(&csched, i, 1);          /* id = index; quantum unused (cooperative) */
        task_init(&ctask[i], cstack[i], CSTACK, coop_worker);
    }
    kprintf("  [coop] starting %d cooperative tasks...\n", NCOOP);
    coop_yield();                          /* enter the first task; returns when all exit */
    kprintf("  [coop] all tasks finished, back in kernel main\n");
}
