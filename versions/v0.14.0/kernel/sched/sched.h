/* THUOS — scheduler glue.
 * Owns the in-kernel run queue using the pure policy in sched_core.c. This is
 * the real, host-tested scheduling policy running live in the kernel. The
 * register/stack context switch (making tasks actually run on the CPU) is the
 * next step and is STAGED until paging is boot-verified, so for now the tasks
 * are scheduling descriptors the policy rotates over. */
#ifndef THUOS_SCHED_H
#define THUOS_SCHED_H

#include "types.h"

void sched_kinit(void);     /* set up the run queue + initial tasks */
int  sched_kcurrent(void);  /* current task id, or -1 */
int  sched_krunnable(void); /* count of runnable tasks */
void sched_klist(void);     /* print the task table + states */
void sched_kdemo(void);     /* run the real RR policy live and print the order */

#endif /* THUOS_SCHED_H */
