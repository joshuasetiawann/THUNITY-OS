/* THUOS — physical memory manager.
 *
 * Strategy: a static bitmap (1 bit per 4 KiB frame) covers up to 4 GiB. At
 * init every frame starts USED; AVAILABLE regions from the Multiboot memory
 * map are then marked FREE; finally the protected regions are reserved again.
 * The bitmap lives in .bss (inside the kernel image), so it is covered by the
 * kernel-image reservation automatically. */
#include "pmm.h"
#include "frame_bitmap.h"
#include "multiboot.h"
#include "kprintf.h"

#define PAGE_SIZE    PMM_PAGE_SIZE
#define MAX_FRAMES   0x100000u           /* 4 GiB / 4 KiB = 1,048,576 frames   */
#define BITMAP_BYTES (MAX_FRAMES / 8u)   /* 128 KiB, lives in .bss             */
#define LOW_MEMORY   0x100000u           /* first 1 MiB is always protected    */

/* Linker-provided bounds of the loaded kernel image (see linker.ld). */
extern uint8_t thuos_kernel_start[];
extern uint8_t thuos_kernel_end[];

static uint8_t  frame_bitmap[BITMAP_BYTES];

static uint32_t total_frames;
static uint32_t usable_frames;
static uint32_t reserved_frames;
static uint32_t usable_bytes;
static uint32_t mem_lower_kb;
static uint32_t mem_upper_kb;
static uint32_t mmap_addr_saved;
static uint32_t mmap_len_saved;
static int      have_map;

/* Recorded protected ranges, so freepage can refuse to touch them. */
#define MAX_PROT 6
static struct { uint32_t start, end; } prot[MAX_PROT];
static int prot_count;

static void reserve_range(uint32_t start, uint32_t end) {
    if (end <= start) return;
    uint32_t f0 = start / PAGE_SIZE;
    uint32_t f1 = (end + PAGE_SIZE - 1) / PAGE_SIZE;   /* round up */
    for (uint32_t f = f0; f < f1 && f < total_frames; f++) {
        if (!fb_test(frame_bitmap, f)) {              /* flip free -> used */
            fb_mark_used(frame_bitmap, f);
            reserved_frames++;
        }
    }
    if (prot_count < MAX_PROT) {
        prot[prot_count].start = start;
        prot[prot_count].end   = end;
        prot_count++;
    }
}

void pmm_init(uint32_t magic, uint32_t mb_info_addr) {
    total_frames = 0; usable_frames = 0; reserved_frames = 0; usable_bytes = 0;
    mem_lower_kb = 0; mem_upper_kb = 0; mmap_addr_saved = 0; mmap_len_saved = 0;
    prot_count = 0; have_map = 0;

    fb_init(frame_bitmap, MAX_FRAMES);   /* everything USED to begin with */

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || mb_info_addr == 0) {
        kprintf("  [warn] PMM: no Multiboot info (magic=0x%08x); inactive\n", magic);
        return;
    }

    multiboot_info_t *mbi = (multiboot_info_t *)mb_info_addr;
    if (mbi->flags & MB_FLAG_MEM) {
        mem_lower_kb = mbi->mem_lower;
        mem_upper_kb = mbi->mem_upper;
    }

    uint32_t highest = 0;   /* highest usable physical address (capped to 4 GiB) */

    if (mbi->flags & MB_FLAG_MMAP) {
        have_map = 1;
        mmap_addr_saved = mbi->mmap_addr;
        mmap_len_saved  = mbi->mmap_length;

        uint32_t ptr = mbi->mmap_addr;
        uint32_t end = mbi->mmap_addr + mbi->mmap_length;
        while (ptr < end) {
            multiboot_mmap_entry_t *e = (multiboot_mmap_entry_t *)ptr;
            if (e->type == MB_MEMORY_AVAILABLE) {
                uint64_t rstart = e->addr;
                uint64_t rend   = e->addr + e->len;
                if (rend > 0x100000000ull) rend = 0x100000000ull;   /* cap 4 GiB */
                if (rend > highest) highest = (uint32_t)rend;
                usable_bytes += (uint32_t)e->len;
                uint32_t f0 = (uint32_t)(rstart / PAGE_SIZE);
                uint32_t f1 = (uint32_t)(rend   / PAGE_SIZE);       /* whole frames */
                for (uint32_t f = f0; f < f1 && f < MAX_FRAMES; f++)
                    fb_mark_free(frame_bitmap, f);
            }
            ptr += e->size + 4;
        }
    } else if (mbi->flags & MB_FLAG_MEM) {
        /* Fallback: assume contiguous RAM from 1 MiB up to mem_upper. */
        highest = LOW_MEMORY + mbi->mem_upper * 1024u;
        usable_bytes = mbi->mem_upper * 1024u;
        uint32_t f0 = LOW_MEMORY / PAGE_SIZE;
        uint32_t f1 = highest / PAGE_SIZE;
        for (uint32_t f = f0; f < f1 && f < MAX_FRAMES; f++)
            fb_mark_free(frame_bitmap, f);
    }

    total_frames = highest / PAGE_SIZE;
    if (total_frames > MAX_FRAMES) total_frames = MAX_FRAMES;
    usable_frames = fb_count_free(frame_bitmap, total_frames);

    /* Reserve protected regions (each is flipped back to USED). */
    reserve_range(0, LOW_MEMORY);                                           /* BIOS/IVT/EBDA/VGA */
    reserve_range((uint32_t)thuos_kernel_start, (uint32_t)thuos_kernel_end);/* kernel + stack + bitmap */
    reserve_range(mb_info_addr, mb_info_addr + sizeof(multiboot_info_t));   /* Multiboot info */
    if (mmap_len_saved)
        reserve_range(mmap_addr_saved, mmap_addr_saved + mmap_len_saved);   /* mmap buffer */
}

uint32_t pmm_alloc_frame(void) {
    if (total_frames == 0) return 0;
    uint32_t f = fb_alloc(frame_bitmap, total_frames);
    if (f == FB_NONE) return 0;
    return f * PAGE_SIZE;
}

int pmm_free_frame(uint32_t phys_addr) {
    if (phys_addr % PAGE_SIZE) return -1;
    uint32_t f = phys_addr / PAGE_SIZE;
    if (f >= total_frames)        return -1;
    if (pmm_is_protected(phys_addr)) return -2;
    if (!fb_test(frame_bitmap, f)) return -3;   /* already free */
    fb_mark_free(frame_bitmap, f);
    return 0;
}

int pmm_is_protected(uint32_t phys_addr) {
    for (int i = 0; i < prot_count; i++)
        if (phys_addr >= prot[i].start && phys_addr < prot[i].end) return 1;
    return 0;
}

uint32_t pmm_total_frames(void)    { return total_frames; }
uint32_t pmm_usable_frames(void)   { return usable_frames; }
uint32_t pmm_reserved_frames(void) { return reserved_frames; }
uint32_t pmm_free_frames(void)     { return fb_count_free(frame_bitmap, total_frames); }
uint32_t pmm_used_frames(void)     { return usable_frames - pmm_free_frames(); }
uint32_t pmm_usable_bytes(void)    { return usable_bytes; }
uint32_t pmm_mem_lower_kb(void)    { return mem_lower_kb; }
uint32_t pmm_mem_upper_kb(void)    { return mem_upper_kb; }
int      pmm_available(void)       { return have_map || (mem_upper_kb != 0); }

static const char *mmap_type_name(uint32_t type) {
    switch (type) {
        case MB_MEMORY_AVAILABLE:        return "available";
        case MB_MEMORY_ACPI_RECLAIMABLE: return "ACPI-reclaim";
        case MB_MEMORY_NVS:              return "ACPI-NVS";
        case MB_MEMORY_BADRAM:           return "bad RAM";
        default:                         return "reserved";
    }
}

void pmm_print_mmap(void) {
    if (!mmap_len_saved) {
        kprintf("No Multiboot memory map was provided by the bootloader.\n");
        if (mem_upper_kb)
            kprintf("Fallback hint: %u KiB low, %u KiB high.\n", mem_lower_kb, mem_upper_kb);
        return;
    }
    kprintf("Multiboot memory map (base / length / type):\n");
    uint32_t ptr = mmap_addr_saved;
    uint32_t end = mmap_addr_saved + mmap_len_saved;
    while (ptr < end) {
        multiboot_mmap_entry_t *e = (multiboot_mmap_entry_t *)ptr;
        kprintf("  0x%08x  0x%08x  %s\n",
                (uint32_t)e->addr, (uint32_t)e->len, mmap_type_name(e->type));
        ptr += e->size + 4;
    }
}

void pmm_print_stats(void) {
    if (total_frames == 0) {
        kprintf("PMM inactive (no usable memory map).\n");
        return;
    }
    uint32_t free_f = pmm_free_frames();
    kprintf("Physical memory (4 KiB frames):\n");
    kprintf("  usable RAM : ~%u KiB (%u frames)\n", usable_bytes / 1024u, usable_frames);
    kprintf("  reserved   : %u frames (low 1 MiB + kernel + boot structs)\n", reserved_frames);
    kprintf("  free       : %u frames (~%u KiB)\n", free_f, free_f * 4u);
    kprintf("  used       : %u frames (reserved + allocated)\n", usable_frames - free_f);
}
