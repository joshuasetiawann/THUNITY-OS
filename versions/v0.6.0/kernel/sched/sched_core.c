/* THUOS — pure scheduler policy (round-robin with time slices). No kernel deps. */
#include "sched_core.h"

void sched_init(sched_t *s, sched_task_t *storage, int cap) {
    s->tasks = storage;
    s->cap = cap;
    s->current = -1;
    s->cursor = 0;
    for (int i = 0; i < cap; i++) {
        storage[i].id = 0;
        storage[i].state = TASK_UNUSED;
        storage[i].quantum = 0;
        storage[i].reload = 0;
        storage[i].ran = 0;
    }
}

static int find_id(sched_t *s, int id) {
    for (int i = 0; i < s->cap; i++)
        if (s->tasks[i].state != TASK_UNUSED && s->tasks[i].id == id) return i;
    return -1;
}

int sched_add(sched_t *s, int id, uint32_t quantum) {
    if (quantum == 0) quantum = 1;
    for (int i = 0; i < s->cap; i++) {
        if (s->tasks[i].state == TASK_UNUSED) {
            s->tasks[i].id = id;
            s->tasks[i].state = TASK_READY;
            s->tasks[i].quantum = quantum;
            s->tasks[i].reload = quantum;
            s->tasks[i].ran = 0;
            return i;
        }
    }
    return -1;   /* table full */
}

int sched_pick_next(sched_t *s) {
    /* The currently running task goes back to the ready pool. */
    if (s->current >= 0 && s->tasks[s->current].state == TASK_RUNNING)
        s->tasks[s->current].state = TASK_READY;

    for (int n = 0; n < s->cap; n++) {
        int i = (s->cursor + n) % s->cap;
        if (s->tasks[i].state == TASK_READY) {
            s->tasks[i].state = TASK_RUNNING;
            s->tasks[i].quantum = s->tasks[i].reload;
            s->current = i;
            s->cursor = (i + 1) % s->cap;
            return s->tasks[i].id;
        }
    }
    s->current = -1;
    return -1;   /* nothing runnable -> idle */
}

int sched_on_tick(sched_t *s) {
    if (s->current < 0) return 0;
    sched_task_t *t = &s->tasks[s->current];
    if (t->state != TASK_RUNNING) return 0;
    t->ran++;
    if (t->quantum > 0) t->quantum--;
    return (t->quantum == 0) ? 1 : 0;   /* slice expired -> caller reschedules */
}

void sched_block(sched_t *s, int id) {
    int i = find_id(s, id);
    if (i < 0) return;
    s->tasks[i].state = TASK_BLOCKED;
    if (s->current == i) s->current = -1;
}

void sched_unblock(sched_t *s, int id) {
    int i = find_id(s, id);
    if (i < 0) return;
    if (s->tasks[i].state == TASK_BLOCKED) s->tasks[i].state = TASK_READY;
}

void sched_exit(sched_t *s, int id) {
    int i = find_id(s, id);
    if (i < 0) return;
    s->tasks[i].state = TASK_ZOMBIE;
    if (s->current == i) s->current = -1;
}

int sched_current(const sched_t *s) {
    return (s->current < 0) ? -1 : s->tasks[s->current].id;
}

int sched_runnable(const sched_t *s) {
    int c = 0;
    for (int i = 0; i < s->cap; i++)
        if (s->tasks[i].state == TASK_READY || s->tasks[i].state == TASK_RUNNING) c++;
    return c;
}

const char *sched_state_name(task_state_t st) {
    switch (st) {
        case TASK_READY:   return "READY";
        case TASK_RUNNING: return "RUNNING";
        case TASK_BLOCKED: return "BLOCKED";
        case TASK_ZOMBIE:  return "ZOMBIE";
        default:           return "UNUSED";
    }
}
