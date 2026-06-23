/* THUOS — kernel panic and assertion system.
 * THUOS never fails silently: unrecoverable faults stop the CPU loudly. */
#ifndef THUOS_PANIC_H
#define THUOS_PANIC_H

void panic(const char *message, const char *file, int line);

#define PANIC(msg) panic((msg), __FILE__, __LINE__)

#define ASSERT(cond)                                              \
    do {                                                          \
        if (!(cond)) panic("assertion failed: " #cond,            \
                           __FILE__, __LINE__);                   \
    } while (0)

#endif /* THUOS_PANIC_H */
