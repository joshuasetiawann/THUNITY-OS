/* THUOS — kernel entry point.
 * Brings up diagnostics, the descriptor tables, interrupt controllers, the
 * timer and the keyboard, then hands control to the THUOS shell. */
#include "types.h"
#include "version.h"
#include "vga.h"
#include "serial.h"
#include "kprintf.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "pit.h"
#include "keyboard.h"
#include "shell.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MB_FLAG_MEM 0x00000001

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;   /* KiB */
    uint32_t mem_upper;   /* KiB */
    /* ... further fields exist but are not needed at this milestone ... */
} __attribute__((packed));

static void ok_line(const char *label) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    kprintf("  [ OK ] ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    kprintf("%s\n", label);
}

void kernel_main(uint32_t magic, uint32_t mb_info_addr) {
    int serial_ok = (serial_init() == 0);

    vga_init();
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    kprintf("%s %s \"%s\"\n", THUOS_NAME, THUOS_VERSION, THUOS_CODENAME);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    kprintf("%s - %s\n", THUOS_KERNEL_NAME, THUOS_ARCH);
    kprintf("From-scratch OS foundation. Booting...\n\n");

    serial_write("\n[THUOS] serial COM1 online\n");

    if (serial_ok) ok_line("Serial COM1 debug channel");
    else           kprintf("  [WARN] Serial COM1 not detected (continuing)\n");

    gdt_init();        ok_line("Global Descriptor Table");
    idt_init();        ok_line("Interrupt Descriptor Table");
    isr_init();        ok_line("CPU exception handlers (0-31)");
    irq_init();        ok_line("PIC remap + IRQ routing");
    pit_init(100);     ok_line("PIT timer @ 100 Hz");
    keyboard_init();   ok_line("PS/2 keyboard (IRQ1)");

    /* Record memory hints from the bootloader if the magic checks out. */
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC && mb_info_addr) {
        struct multiboot_info *mbi = (struct multiboot_info *)mb_info_addr;
        if (mbi->flags & MB_FLAG_MEM) {
            shell_set_memory(mbi->mem_lower, mbi->mem_upper);
            kprintf("  [info] Multiboot memory: %u KiB low, %u KiB high\n",
                    mbi->mem_lower, mbi->mem_upper);
        }
    } else {
        kprintf("  [info] No valid Multiboot info (magic=0x%08x)\n", magic);
    }

    __asm__ volatile("sti");   /* interrupts on: timer + keyboard now live */

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    kprintf("\nWelcome to %s.\n", THUOS_NAME);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    shell_run();   /* never returns */

    for (;;) __asm__ volatile("hlt");
}
