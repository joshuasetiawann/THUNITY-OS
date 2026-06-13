/* THUOS — pure in-memory filesystem (ramfs) core. No kernel/hardware deps, so
 * the file-table logic is unit-tested on the host (tests/test_fs.c). Files live
 * in a caller-provided entry table + byte arena; writes bump-allocate fresh
 * space in the arena (simple, no per-file free). This is the local-first
 * storage foundation; later it can be backed by an initrd / persistent store. */
#ifndef THUOS_RAMFS_CORE_H
#define THUOS_RAMFS_CORE_H

#ifdef THUOS_HOSTED_TEST
#include <stdint.h>
#include <stddef.h>
#else
#include "types.h"
#endif

#define RFS_NAME_MAX 32

typedef struct {
    char     name[RFS_NAME_MAX];
    uint32_t off;     /* offset into the arena */
    uint32_t size;    /* content length in bytes */
    int      used;
} rfile_t;

typedef struct {
    rfile_t *files;
    int      max_files;
    int      count;
    uint8_t *arena;
    uint32_t arena_sz;
    uint32_t arena_used;
} ramfs_t;

void        ramfs_init(ramfs_t *fs, rfile_t *files, int max_files,
                       uint8_t *arena, uint32_t arena_sz);
int         ramfs_find(const ramfs_t *fs, const char *name);          /* index or -1 */
/* Create-or-replace a file's content; returns bytes written, or -1 on no space. */
int         ramfs_write(ramfs_t *fs, const char *name, const void *data, uint32_t len);
const uint8_t *ramfs_read(const ramfs_t *fs, const char *name, uint32_t *out_len);
int         ramfs_count(const ramfs_t *fs);
const char *ramfs_name(const ramfs_t *fs, int i);
uint32_t    ramfs_size(const ramfs_t *fs, int i);
uint32_t    ramfs_bytes_used(const ramfs_t *fs);

#endif /* THUOS_RAMFS_CORE_H */
