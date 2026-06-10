/* THUOS — Multiboot 1 information structures (the parts THUOS reads).
 * Reference: Multiboot Specification 0.6.96. */
#ifndef THUOS_MULTIBOOT_H
#define THUOS_MULTIBOOT_H

#include "types.h"

/* Value the bootloader leaves in EAX when it hands control to the kernel. */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/* multiboot_info_t.flags bits we care about. */
#define MB_FLAG_MEM    0x00000001   /* mem_lower / mem_upper valid */
#define MB_FLAG_MMAP   0x00000040   /* mmap_length / mmap_addr valid */
#define MB_FLAG_FB     0x00001000   /* framebuffer_* fields valid (loader set a mode) */

/* framebuffer_type values. */
#define MB_FB_TYPE_INDEXED 0
#define MB_FB_TYPE_RGB     1        /* direct-colour: what THUOS draws into */
#define MB_FB_TYPE_TEXT    2

/* multiboot_mmap_entry_t.type values. */
#define MB_MEMORY_AVAILABLE        1
#define MB_MEMORY_RESERVED         2
#define MB_MEMORY_ACPI_RECLAIMABLE 3
#define MB_MEMORY_NVS              4
#define MB_MEMORY_BADRAM           5

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;     /* KiB of lower memory  */
    uint32_t mem_upper;     /* KiB of upper memory  */
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;   /* bytes of the memory-map buffer */
    uint32_t mmap_addr;     /* physical address of the buffer */
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;    /* physical address of the linear framebuffer */
    uint32_t framebuffer_pitch;   /* bytes per scanline (often > width*bpp/8!) */
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;     /* bits per pixel */
    uint8_t  framebuffer_type;    /* MB_FB_TYPE_* */
    uint8_t  color_info[6];
} __attribute__((packed)) multiboot_info_t;

/* One entry in the memory map. NOTE: `size` does not include itself, and
 * the next entry is at (uint8_t*)entry + entry->size + 4. */
typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

#endif /* THUOS_MULTIBOOT_H */
