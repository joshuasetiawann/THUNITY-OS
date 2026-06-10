/* THUOS — pure in-memory filesystem (ramfs) core. No kernel dependencies. */
#include "ramfs_core.h"

/* Local string helpers (kept private so the core links standalone in tests). */
static int rfs_streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
static void rfs_ncopy(char *dst, const char *src, int max) {
    int i = 0;
    for (; i < max - 1 && src[i]; i++) dst[i] = src[i];
    dst[i] = '\0';
}

void ramfs_init(ramfs_t *fs, rfile_t *files, int max_files,
                uint8_t *arena, uint32_t arena_sz) {
    fs->files = files;
    fs->max_files = max_files;
    fs->count = 0;
    fs->arena = arena;
    fs->arena_sz = arena_sz;
    fs->arena_used = 0;
    for (int i = 0; i < max_files; i++) { files[i].used = 0; files[i].name[0] = '\0'; }
}

int ramfs_find(const ramfs_t *fs, const char *name) {
    for (int i = 0; i < fs->max_files; i++)
        if (fs->files[i].used && rfs_streq(fs->files[i].name, name)) return i;
    return -1;
}

static int ramfs_alloc_slot(ramfs_t *fs, const char *name) {
    int idx = ramfs_find(fs, name);
    if (idx >= 0) return idx;
    for (int i = 0; i < fs->max_files; i++) {
        if (!fs->files[i].used) {
            fs->files[i].used = 1;
            rfs_ncopy(fs->files[i].name, name, RFS_NAME_MAX);
            fs->files[i].off = 0;
            fs->files[i].size = 0;
            fs->count++;
            return i;
        }
    }
    return -1;   /* file table full */
}

int ramfs_write(ramfs_t *fs, const char *name, const void *data, uint32_t len) {
    if (name[0] == '\0') return -1;
    if (fs->arena_used + len > fs->arena_sz) return -1;   /* out of arena space */
    int idx = ramfs_alloc_slot(fs, name);
    if (idx < 0) return -1;

    uint32_t off = fs->arena_used;                        /* bump-allocate */
    const uint8_t *src = (const uint8_t *)data;
    for (uint32_t i = 0; i < len; i++) fs->arena[off + i] = src[i];
    fs->arena_used += len;

    fs->files[idx].off = off;
    fs->files[idx].size = len;
    return (int)len;
}

const uint8_t *ramfs_read(const ramfs_t *fs, const char *name, uint32_t *out_len) {
    int idx = ramfs_find(fs, name);
    if (idx < 0) { if (out_len) *out_len = 0; return NULL; }
    if (out_len) *out_len = fs->files[idx].size;
    return fs->arena + fs->files[idx].off;
}

int         ramfs_count(const ramfs_t *fs)        { return fs->count; }
uint32_t    ramfs_bytes_used(const ramfs_t *fs)   { return fs->arena_used; }

const char *ramfs_name(const ramfs_t *fs, int i) {
    if (i < 0 || i >= fs->max_files || !fs->files[i].used) return (const char *)0;
    return fs->files[i].name;
}
uint32_t ramfs_size(const ramfs_t *fs, int i) {
    if (i < 0 || i >= fs->max_files || !fs->files[i].used) return 0;
    return fs->files[i].size;
}
