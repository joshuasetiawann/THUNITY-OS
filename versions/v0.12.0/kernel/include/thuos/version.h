/* THUOS — single source of truth for project identity strings. */
#ifndef THUOS_VERSION_H
#define THUOS_VERSION_H

#define THUOS_NAME         "THUOS"
#define THUOS_KERNEL_NAME  "THU Kernel"
#define THUOS_DESKTOP_NAME "THU Desktop"
#define THUOS_FS_NAME      "THUFS"
#define THUOS_PKG_NAME     "thupkg"

#define THUOS_VERSION      "0.12.0"
#define THUOS_CODENAME     "User Mode"
#define THUOS_MILESTONE    "Milestone 0.12 - User mode (ring 3): TSS + iret to CPL 3 + int 0x80 from userspace, host-tested + boot-verified (QEMU/CI)"
#define THUOS_ARCH         "x86 (i386, 32-bit)"

#define THUOS_BUILD_DATE   __DATE__
#define THUOS_BUILD_TIME   __TIME__

#endif /* THUOS_VERSION_H */
