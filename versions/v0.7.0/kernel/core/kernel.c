/* THUOS — kernel entry point.
 * Brings up diagnostics, the descriptor tables, interrupt controllers, the
 * timer, the keyboard, and the physical memory manager, then hands control to
 * the THUOS shell. */
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
#include "pmm.h"
#include "kheap.h"
#include "vmm.h"
#include "sched.h"
#include "shell.h"

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

    pmm_init(magic, mb_info_addr);
    ok_line("Physical memory manager (4 KiB frames)");
    pmm_print_stats();

    kheap_init();
    ok_line("Kernel heap (1 MiB fixed arena, kmalloc/kfree)");

    vmm_init();
    ok_line("Paging tables: identity map low 8 MiB built");
    vmm_enable();
    ok_line("Paging ENABLED (CR0.PG) - running under virtual memory");

    sched_kinit();
    ok_line("Scheduler: round-robin policy core (context switch staged)");

    __asm__ volatile("sti");   /* interrupts on: timer + keyboard now live */

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    kprintf("\nWelcome to %s.\n", THUOS_NAME);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    shell_run();   /* never returns */

    for (;;) __asm__ volatile("hlt");
}
