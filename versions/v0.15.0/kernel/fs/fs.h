/* THUOS — kernel filesystem facade over the ramfs core.
 * A static in-RAM filesystem (file table + 64 KiB arena) seeded with a couple
 * of built-in files at boot. Local-first storage you can ls/cat/write from the
 * shell. Host-tested core (tests/test_fs.c) + boot-verified. */
#ifndef THUOS_FS_H
#define THUOS_FS_H

#include "types.h"

void           kfs_init(void);
int            kfs_write(const char *name, const void *data, uint32_t len); /* bytes or -1 */
const uint8_t *kfs_read(const char *name, uint32_t *out_len);
int            kfs_count(void);
const char    *kfs_name(int i);     /* NULL if slot empty/out of range */
uint32_t       kfs_size(int i);
uint32_t       kfs_bytes_used(void);
int            kfs_max_files(void);

#endif /* THUOS_FS_H */
