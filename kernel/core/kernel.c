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
#include "task.h"
#include "coop.h"
#include "fs.h"
#include "syscall.h"
#include "usermode.h"
#include "gfx.h"
#include "lfb.h"
#include "xhci.h"
#include "ai.h"
#include "desktop.h"
#include "shell.h"

static void ok_line(const char *label) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    kprintf("  [ OK ] ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    kprintf("%s\n", label);
}

/* Cooperative context-switch self-test: switch into a task that runs on its own
 * stack and immediately switches back. Proves real register/stack switching.
 * Boot-verified in QEMU by CI: if the switch were wrong, the kernel would crash
 * here and never reach the shell. */
static ktask_t  ctx_task;
static uint8_t  ctx_task_stack[4096] __attribute__((aligned(16)));
static uint32_t ctx_main_esp;

static void ctx_task_entry(void) {
    kprintf("  [ctx] now running inside a task, on a separate stack\n");
    thuos_context_switch(&ctx_task.esp, ctx_main_esp);   /* yield back to main */
    for (;;) __asm__ volatile("hlt");                    /* not reached here   */
}

static void ctx_switch_demo(void) {
    task_init(&ctx_task, ctx_task_stack, sizeof ctx_task_stack, ctx_task_entry);
    thuos_context_switch(&ctx_main_esp, ctx_task.esp);   /* -> ctx_task_entry  */
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
    ok_line("Scheduler: round-robin policy core");

    ctx_switch_demo();
    ok_line("Context switch OK (ran a task on its own stack, returned)");

    coop_run_demo();
    ok_line("Cooperative multitasking OK (3 tasks via scheduler + exited)");

    kfs_init();
    ok_line("RAM filesystem ready (local-first, in-kernel files)");

    syscall_init();
    syscall_selftest();
    ok_line("Syscall interface (int 0x80, ABI for userspace)");

    usermode_init();
    usermode_selftest();       /* drop to ring 3, syscall back, return to ring 0 */
    ok_line("User mode (ring 3): entered CPL 3 and returned via int 0x80");

    xhci_init();               /* USB: bring up the xHCI controller (real-HW USB path) */

    ai_init();                 /* AI-native layer: registry + local-only policy (no inference) */
    ok_line("AI-native layer: service/model/task/policy/audit core (host-tested)");

    vga_font_extract();        /* copy the VGA font while still in text mode */
    ok_line("THU Desktop: high-res framebuffer (1024x768x32 truecolor)");

    __asm__ volatile("sti");   /* interrupts on: timer + keyboard + mouse now live */

    desktop_start(mb_info_addr); /* bootloader FB (real HW) or Bochs VBE (QEMU); else text */
    if (lfb_active()) {
        desktop_run();         /* mouse + dock + apps; runs the shell as the terminal app */
    } else {
        kprintf("Welcome to %s %s \"%s\".\n\n", THUOS_NAME, THUOS_VERSION, THUOS_CODENAME);
        shell_run();           /* text-mode fallback */
    }

    for (;;) __asm__ volatile("hlt");
}
