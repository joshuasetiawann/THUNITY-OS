/* THUOS — kernel panic and assertion system. */
#include "panic.h"
#include "kprintf.h"
#include "vga.h"

void panic(const char *message, const char *file, int line) {
    __asm__ volatile("cli");

    vga_set_color(VGA_WHITE, VGA_RED);
    vga_clear();
    kprintf("\n");
    kprintf("  *** THUOS KERNEL PANIC ***\n\n");
    kprintf("  %s\n\n", message);
    kprintf("  at %s:%d\n\n", file, line);
    kprintf("  The system has been halted to prevent damage.\n");
    kprintf("  This is an honest stop, not a fake recovery.\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}
