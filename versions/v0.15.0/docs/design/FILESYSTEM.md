# THUOS Filesystem — VFS, initrd, devfs, and the road to THUFS

**Status: Designed / Planned.** No filesystem code is built or linked into
THU Kernel yet. This document describes the layered plan that the kernel will
grow into. The current 0.2.0 "Boot Seed" kernel has no `open`, `read`, `ls`,
`cat`, or mount support — those shell commands are not present today.

> Honesty note: everything below is a forward-looking design. When a piece of
> this is actually implemented and build-verified, its row in the status table
> will be promoted from **Planned** / **Designed** to **Partially implemented**
> or **Implemented**, and the relevant shell commands will appear in `help`.

---

## 1. Why a VFS first

THUOS deliberately designs the **Virtual File System (VFS)** abstraction before
committing to any on-disk format. The VFS is the single, stable interface that
the shell, future userspace, drivers, and `thupkg` all talk to. Concrete
backends (ramfs, devfs, a procfs-style view, and eventually THUFS) plug in
underneath it. This keeps storage decisions reversible and keeps higher layers
honest: a path like `/dev/com1` or `/proc/uptime` behaves the same regardless
of where its bytes come from.

```text
  shell / userspace / thupkg
            |
            v
        +-------+        one stable API: open/read/write/close, lookup, readdir
        |  VFS  |
        +-------+
        /   |   \
   ramfs  devfs  procfs-style ...  -> later: THUFS on a real block device
```

## 2. Layer roadmap

| Layer                     | Purpose                                            | Status   | Target milestone |
|---------------------------|----------------------------------------------------|----------|------------------|
| VFS abstraction           | inode / file / dirent / mount interface            | Designed | 0.4 VFS Seed     |
| initrd / ramfs            | in-memory root for early boot                      | Designed | 0.4 VFS Seed     |
| devfs                     | device nodes (`/dev/...`)                           | Designed | 0.4 / 0.5        |
| procfs-style introspection| read-only kernel state (`/proc/...`)               | Designed | 0.4 / 0.5        |
| Path resolution + mounts  | `/`-rooted tree, mountpoint traversal              | Planned  | 0.4 VFS Seed     |
| Block layer + ATA/IDE     | real persistent storage                            | Planned  | 0.6+             |
| THUFS                     | THUOS-native on-disk filesystem (see `THUFS_SPEC`) | Designed | 1.0 horizon      |

## 3. Core objects (Designed)

The VFS models four small, explicit object types. Signatures below are the
intended shape and follow the existing kernel style (`kernel/lib/types.h`,
freestanding C, no host libc).

```c
/* A node in the tree: file, directory, or device. (Designed, not built.) */
typedef enum { VN_FILE, VN_DIR, VN_DEV, VN_SYMLINK } vnode_type_t;

struct vnode {
    vnode_type_t      type;
    uint32_t          id;        /* inode number, unique within a mount     */
    uint32_t          size;      /* bytes for files                          */
    uint16_t          mode;      /* rwx permission bits (see SECURITY_MODEL)  */
    struct vfs_ops   *ops;       /* backend implementation                   */
    void             *priv;      /* backend-private data (ramfs node, etc.)  */
    struct mount     *mnt;       /* owning mount                             */
};

/* Per-backend operation table. A backend implements only what it supports;
 * unsupported operations return an explicit error code, never silent zero. */
struct vfs_ops {
    int  (*lookup)(struct vnode *dir, const char *name, struct vnode **out);
    int  (*read)  (struct vnode *vn, uint32_t off, void *buf, uint32_t n);
    int  (*write) (struct vnode *vn, uint32_t off, const void *buf, uint32_t n);
    int  (*readdir)(struct vnode *dir, uint32_t idx, char *name_out, uint32_t cap);
    int  (*create)(struct vnode *dir, const char *name, vnode_type_t t,
                   struct vnode **out);
};

/* An open handle held by the shell or a future process. */
struct file {
    struct vnode *vn;
    uint32_t      pos;
    uint32_t      flags;   /* O_READ / O_WRITE / O_APPEND ...               */
};

/* A backend bound into the tree at a path. */
struct mount {
    char            point[64];   /* e.g. "/", "/dev", "/proc"               */
    struct vfs_ops *ops;
    struct vnode   *root;
    void           *fs_priv;
};
```

Design rules carried over from the kernel's existing conventions:

- **No silent failure.** Every VFS call returns a negative error code on
  failure (mirroring `serial_init()` returning `int`). Callers must check it.
- **Explicit types.** Fixed-width integers from `types.h`; no host headers.
- **Serial-logged faults.** VFS errors during boot are echoed to COM1 via the
  existing `serial_write` path so they are visible even before any console UI.

## 4. initrd / ramfs (Designed)

The first backend is an in-memory root, populated from an **initrd** image that
the bootloader (GRUB, via Multiboot) hands to the kernel as a module. This gives
THUOS a usable `/` before any storage driver exists.

```text
Boot flow (planned):
  GRUB loads kernel.elf  +  initrd module
        |
        v
  kernel_main() reads Multiboot module list   (today only mem_* is parsed)
        |
        v
  ramfs_mount("/")  <-- unpacks initrd archive into vnodes in RAM
        |
        v
  shell can ls / cat files under "/"
```

The initrd archive format will be a minimal, self-describing record stream
(name, type, length, bytes) — deliberately simpler than tar for a first cut, and
documented before any tool emits it. ramfs is read-mostly; writes live only in
RAM and are lost on reboot, which is intentional and safe for emulator-first
development (no disk is touched).

## 5. devfs (Designed)

`devfs` exposes kernel drivers as file-like nodes so userspace and the shell can
reach hardware through the same `read`/`write` API. The nodes below map onto
drivers that are **already implemented** in the kernel (VGA, serial, keyboard,
PIT) plus planned ones.

| Path           | Backing driver           | Driver status today        |
|----------------|--------------------------|----------------------------|
| `/dev/console` | VGA text console         | Implemented (driver)       |
| `/dev/com1`    | COM1 serial              | Implemented (driver)       |
| `/dev/kbd`     | PS/2 keyboard (IRQ1)     | Implemented (driver)       |
| `/dev/timer`   | PIT @ 100 Hz             | Implemented (driver)       |
| `/dev/null`    | sink                     | Planned                    |
| `/dev/ata0`    | ATA/IDE disk             | Planned                    |
| `/dev/mouse`   | PS/2 mouse               | Planned                    |

Note the distinction: the *drivers* exist and run today, but the *devfs surface*
that turns them into `/dev/...` paths is **Designed**, not built. See
`05_DRIVER_MODEL.md` for the driver interface itself.

## 6. procfs-style introspection (Designed)

A read-only, synthetic tree exposes kernel state as text — the file-based twin of
the existing `sysinfo`, `uptime`, `ticks`, and `mem` shell commands. Nothing is
stored; each read renders live values.

```text
/proc/version    ->  THU Kernel 0.2.0 "Boot Seed"
/proc/uptime     ->  <seconds> (from pit_seconds())
/proc/ticks      ->  <raw PIT tick counter>
/proc/meminfo    ->  Multiboot mem_lower / mem_upper hint
/proc/drivers    ->  list of registered drivers and their state
```

This keeps THUOS auditable: anything the kernel knows about itself is readable as
plain text, with no hidden state.

## 7. Path toward THUFS

Once a block layer and an ATA/IDE driver land, the VFS gains a persistent
backend: **THUFS**, the THUOS-native filesystem. Its detailed design lives in
[`THUFS_SPEC.md`](./THUFS_SPEC.md). The sequencing is deliberate:

```text
VFS  ->  ramfs (initrd)  ->  devfs + procfs  ->  block layer + ATA/IDE  ->  THUFS
(stable API first, persistence last, so nothing on a real disk is risked early)
```

Until THUFS exists, THUOS performs **no writes to any real disk or partition**.
All filesystem activity is in RAM (ramfs) or against emulator disk images only.

---

## Status summary

VFS, initrd/ramfs, devfs, and procfs-style introspection are **Designed**; the
block layer and THUFS are **Planned**. No filesystem is implemented in the 0.2.0
kernel yet, and THUOS writes to no real disk.
