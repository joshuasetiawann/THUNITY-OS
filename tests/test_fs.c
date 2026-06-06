/* THUOS — host unit test for the ramfs core (ramfs_core.c). Build & run: make test */
#define THUOS_HOSTED_TEST
#include "../kernel/fs/ramfs_core.c"

#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    static rfile_t files[8];
    static unsigned char arena[4096];
    ramfs_t fs;

    ramfs_init(&fs, files, 8, arena, sizeof arena);
    assert(ramfs_count(&fs) == 0);
    assert(ramfs_find(&fs, "x") == -1);
    assert(ramfs_read(&fs, "x", NULL) == NULL);

    /* write + read back */
    const char *hello = "hello world";
    assert(ramfs_write(&fs, "a.txt", hello, 11) == 11);
    assert(ramfs_count(&fs) == 1);
    uint32_t n;
    const uint8_t *d = ramfs_read(&fs, "a.txt", &n);
    assert(d && n == 11 && memcmp(d, hello, 11) == 0);

    /* a second file */
    assert(ramfs_write(&fs, "b", "xy", 2) == 2);
    assert(ramfs_count(&fs) == 2);
    assert(ramfs_size(&fs, ramfs_find(&fs, "b")) == 2);

    /* create-or-replace keeps the file count and updates content */
    assert(ramfs_write(&fs, "a.txt", "ZZZZZ", 5) == 5);
    assert(ramfs_count(&fs) == 2);
    d = ramfs_read(&fs, "a.txt", &n);
    assert(n == 5 && memcmp(d, "ZZZZZ", 5) == 0);

    /* arena exhaustion is refused (length checked before any copy) */
    assert(ramfs_write(&fs, "big", arena, 5000) == -1);

    /* listing exposes both names */
    int seen_a = 0, seen_b = 0;
    for (int i = 0; i < 8; i++) {
        const char *nm = ramfs_name(&fs, i);
        if (nm) { if (!strcmp(nm, "a.txt")) seen_a = 1; if (!strcmp(nm, "b")) seen_b = 1; }
    }
    assert(seen_a && seen_b);

    printf("ramfs test: OK\n");
    printf("  create/write/read, replace, listing, arena-exhaustion guard\n");
    return 0;
}
