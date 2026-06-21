/* THUOS — Global Descriptor Table.
 * Flat 32-bit memory model: kernel + user code/data segments spanning 4 GiB,
 * plus a TSS descriptor so the CPU has a kernel stack for ring 3 -> ring 0. */
#include "gdt.h"
#include "tss.h"
#include "types.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[6];
static struct gdt_ptr   gdtp;

/* Defined in gdt_flush.S: loads the GDTR and reloads the segment registers. */
extern void gdt_flush(uint32_t gdt_ptr_addr);

static void gdt_set_gate(int n, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran) {
    gdt[n].base_low    = (uint16_t)(base & 0xFFFF);
    gdt[n].base_mid    = (uint8_t)((base >> 16) & 0xFF);
    gdt[n].base_high   = (uint8_t)((base >> 24) & 0xFF);
    gdt[n].limit_low   = (uint16_t)(limit & 0xFFFF);
    gdt[n].granularity = (uint8_t)(((limit >> 16) & 0x0F) | (gran & 0xF0));
    gdt[n].access      = access;
}

void gdt_init(void) {
    gdtp.limit = (uint16_t)(sizeof(gdt) - 1);
    gdtp.base  = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                /* null descriptor      */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /* kernel code (ring 0) */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); /* kernel data (ring 0) */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); /* user code   (ring 3) */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); /* user data   (ring 3) */

    /* TSS: byte-granular system descriptor, access 0x89 (present, DPL 0,
     * available 32-bit TSS). Fill the TSS first so its base/limit are known. */
    tss_init();
    gdt_set_gate(5, tss_base(), tss_limit(), 0x89, 0x00);

    gdt_flush((uint32_t)&gdtp);
    tss_flush(tss_selector());                  /* ltr: load the task register */
}
