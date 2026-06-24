/* THUOS — cooperative multitasking demo: ties the pure scheduler policy
 * (sched_core) to real execution contexts (task.c + context.S) so several
 * kernel tasks actually run on their own stacks and yield to one another via
 * the scheduler. Boot-verified in QEMU by CI. */
#ifndef THUOS_COOP_H
#define THUOS_COOP_H

void coop_run_demo(void);   /* run a few cooperative tasks to completion, then return */

#endif /* THUOS_COOP_H */
