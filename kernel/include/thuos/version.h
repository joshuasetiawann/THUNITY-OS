/* THUOS — single source of truth for project identity strings. */
#ifndef THUOS_VERSION_H
#define THUOS_VERSION_H

#define THUOS_NAME         "THUOS"
#define THUOS_KERNEL_NAME  "THU Kernel"
#define THUOS_DESKTOP_NAME "THU Desktop"
#define THUOS_FS_NAME      "THUFS"
#define THUOS_PKG_NAME     "thupkg"

#define THUOS_VERSION      "0.19.0"
#define THUOS_CODENAME     "USB"
#define THUOS_MILESTONE    "Milestone 0.19 - USB: xHCI host controller + USB-HID boot keyboard & mouse, so input works on real laptops that have no PS/2; PS/2 still supported; boot-verified (QEMU + GRUB ISO)"
#define THUOS_ARCH         "x86 (i386, 32-bit)"

#define THUOS_BUILD_DATE   __DATE__
#define THUOS_BUILD_TIME   __TIME__

#endif /* THUOS_VERSION_H */
