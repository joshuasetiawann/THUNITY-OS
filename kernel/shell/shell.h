/* THUOS — interactive kernel shell. */
#ifndef THUOS_SHELL_H
#define THUOS_SHELL_H

#include "types.h"

/* Memory hints discovered at boot (from the Multiboot info), in KiB. */
void shell_set_memory(uint32_t mem_lower_kb, uint32_t mem_upper_kb);

/* Runs the THUOS shell loop forever. Never returns. */
void shell_run(void);

#endif /* THUOS_SHELL_H */
