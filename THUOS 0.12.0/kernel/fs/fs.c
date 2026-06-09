/* THUOS — kernel filesystem facade (static ramfs). See fs.h. */
#include "fs.h"
#include "ramfs_core.h"

#define KFS_MAX_FILES 32
#define KFS_ARENA     (64u * 1024u)   /* 64 KiB of file content */

static rfile_t kfiles[KFS_MAX_FILES];
static uint8_t karena[KFS_ARENA];
static ramfs_t kfs;

static uint32_t slen(const char *s) { uint32_t n = 0; while (s[n]) n++; return n; }

void kfs_init(void) {
    ramfs_init(&kfs, kfiles, KFS_MAX_FILES, karena, KFS_ARENA);

    const char *welcome =
        "Welcome to the THUOS RAM filesystem.\n"
        "These are real files living in kernel memory.\n"
        "Try:  ls   |   cat readme   |   write notes hello there\n";
    kfs_write("welcome.txt", welcome, slen(welcome));

    const char *readme =
        "THUOS local-first ramfs (milestone 0.10).\n"
        "Host-tested core + boot-verified in QEMU.\n"
        "Files are in-memory for now; persistence comes later.\n";
    kfs_write("readme", readme, slen(readme));
}

int            kfs_write(const char *name, const void *data, uint32_t len) { return ramfs_write(&kfs, name, data, len); }
const uint8_t *kfs_read(const char *name, uint32_t *out_len)               { return ramfs_read(&kfs, name, out_len); }
int            kfs_count(void)        { return ramfs_count(&kfs); }
const char    *kfs_name(int i)        { return ramfs_name(&kfs, i); }
uint32_t       kfs_size(int i)        { return ramfs_size(&kfs, i); }
uint32_t       kfs_bytes_used(void)   { return ramfs_bytes_used(&kfs); }
int            kfs_max_files(void)    { return KFS_MAX_FILES; }
