/* THUOS — pure scheduler policy. No kernel/hardware dependency, so the run
 * queue and round-robin selection the kernel relies on can be unit-tested on
 * the host (see tests/test_sched.c). This is policy only: it decides *who runs
 * next*; the actual register/stack context switch is kernel glue (staged until
 * paging is boot-verified). */
#ifndef THUOS_SCHED_CORE_H
#define THUOS_SCHED_CORE_H

#ifdef THUOS_HOSTED_TEST
#include <stdint.h>
#include <stddef.h>
#else
#include "types.h"
#endif

typedef enum {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

typedef struct {
    int          id;
    task_state_t state;
    uint32_t     quantum;   /* ticks left in the current time slice */
    uint32_t     reload;    /* slice length, reloaded when scheduled */
    uint32_t     ran;       /* total ticks consumed (stats) */
} sched_task_t;

typedef struct {
    sched_task_t *tasks;
    int           cap;
    int           current;  /* index of the running task, or -1 */
    int           cursor;   /* round-robin scan position */
} sched_t;

void        sched_init(sched_t *s, sched_task_t *storage, int cap);
int         sched_add(sched_t *s, int id, uint32_t quantum);  /* slot idx or -1 */
int         sched_pick_next(sched_t *s);     /* id of next task to run, or -1 */
int         sched_on_tick(sched_t *s);       /* 1 if the time slice expired */
void        sched_block(sched_t *s, int id);
void        sched_unblock(sched_t *s, int id);
void        sched_exit(sched_t *s, int id);
int         sched_current(const sched_t *s); /* current id, or -1 */
int         sched_runnable(const sched_t *s);/* count READY + RUNNING */
const char *sched_state_name(task_state_t st);

#endif /* THUOS_SCHED_CORE_H */
