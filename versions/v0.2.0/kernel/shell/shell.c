/* THUOS — interactive kernel shell.
 * Reads lines from the keyboard and dispatches a small set of built-in
 * commands. This is the real shell that runs inside the kernel under an
 * emulator; the browser preview mirrors these same commands for design. */
#include "shell.h"
#include "kprintf.h"
#include "keyboard.h"
#include "vga.h"
#include "io.h"
#include "string.h"
#include "version.h"
#include "pit.h"

#define LINE_MAX 128

static uint32_t mem_lower_kb;
static uint32_t mem_upper_kb;

void shell_set_memory(uint32_t lower_kb, uint32_t upper_kb) {
    mem_lower_kb = lower_kb;
    mem_upper_kb = upper_kb;
}

static void print_banner(void) {
    kprintf("\n");
    kprintf("  ######  ##  ##  ##  ##   ####    #####\n");
    kprintf("    ##    ##  ##  ##  ##  ##  ##  ##\n");
    kprintf("    ##    ######  ##  ##  ##  ##   ####\n");
    kprintf("    ##    ##  ##  ##  ##  ##  ##      ##\n");
    kprintf("    ##    ##  ##   ####    ####   #####\n");
    kprintf("\n  %s %s \"%s\" - %s\n\n",
            THUOS_NAME, THUOS_VERSION, THUOS_CODENAME, THUOS_ARCH);
}

static void cmd_help(void) {
    kprintf("THUOS shell commands:\n");
    kprintf("  help      Show this help\n");
    kprintf("  about     About THUOS\n");
    kprintf("  version   Kernel name and version\n");
    kprintf("  status    Implemented / in-progress / planned\n");
    kprintf("  sysinfo   System summary\n");
    kprintf("  uptime    Uptime in ticks and seconds\n");
    kprintf("  ticks     Raw PIT tick counter\n");
    kprintf("  mem       Memory information (from Multiboot)\n");
    kprintf("  echo      Print the rest of the line\n");
    kprintf("  banner    Show the THUOS banner\n");
    kprintf("  color N   Set text color (0-15)\n");
    kprintf("  thupkg    Package manager (design preview)\n");
    kprintf("  clear     Clear the screen\n");
    kprintf("  crash X   Test fault handler (div0) - testing only\n");
    kprintf("  reboot    Reboot the machine\n");
    kprintf("  halt      Halt the CPU\n");
}

static void cmd_about(void) {
    kprintf("%s - a from-scratch personal operating system.\n", THUOS_NAME);
    kprintf("Kernel : %s\n", THUOS_KERNEL_NAME);
    kprintf("Goal   : local-first, privacy-first, honest engineering.\n");
    kprintf("Built incrementally from a bootable kernel toward %s.\n",
            THUOS_DESKTOP_NAME);
}

static void cmd_version(void) {
    kprintf("%s %s \"%s\"\n", THUOS_KERNEL_NAME, THUOS_VERSION, THUOS_CODENAME);
    kprintf("%s\n", THUOS_MILESTONE);
    kprintf("Built %s %s\n", THUOS_BUILD_DATE, THUOS_BUILD_TIME);
}

static void cmd_status(void) {
    kprintf("THUOS subsystem status (honest):\n");
    kprintf("  [done]    Multiboot boot, VGA console, serial COM1\n");
    kprintf("  [done]    GDT, IDT, CPU exceptions 0-31\n");
    kprintf("  [done]    PIC remap, PIT timer, keyboard IRQ, shell\n");
    kprintf("  [done]    Panic/assert system\n");
    kprintf("  [wip]     Memory manager (Milestone 0.3)\n");
    kprintf("  [plan]    VFS + initrd (Milestone 0.4)\n");
    kprintf("  [plan]    Userspace + syscalls (Milestone 0.5)\n");
    kprintf("  [plan]    Framebuffer GUI / THU Desktop\n");
}

static void cmd_sysinfo(void) {
    kprintf("%s %s \"%s\"\n", THUOS_NAME, THUOS_VERSION, THUOS_CODENAME);
    kprintf("Arch    : %s\n", THUOS_ARCH);
    kprintf("Kernel  : %s\n", THUOS_KERNEL_NAME);
    kprintf("Timer   : PIT @ %u Hz\n", pit_frequency());
    kprintf("Uptime  : %u s (%u ticks)\n", pit_seconds(), pit_ticks());
    if (mem_upper_kb) {
        kprintf("Memory  : ~%u KiB low, ~%u KiB high (Multiboot)\n",
                mem_lower_kb, mem_upper_kb);
    }
    kprintf("Build   : %s %s\n", THUOS_BUILD_DATE, THUOS_BUILD_TIME);
}

static void cmd_uptime(void) {
    kprintf("Uptime: %u ticks, approx %u seconds (PIT @ %u Hz)\n",
            pit_ticks(), pit_seconds(), pit_frequency());
}

static void cmd_mem(void) {
    if (mem_upper_kb) {
        uint32_t total_kb = mem_lower_kb + mem_upper_kb;
        kprintf("Multiboot memory hint:\n");
        kprintf("  lower : %u KiB\n", mem_lower_kb);
        kprintf("  upper : %u KiB\n", mem_upper_kb);
        kprintf("  total : ~%u KiB (~%u MiB)\n", total_kb, total_kb / 1024);
    } else {
        kprintf("No Multiboot memory map was provided by the bootloader.\n");
    }
    kprintf("Full physical memory manager is planned for Milestone 0.3.\n");
}

static void cmd_thupkg(const char *args) {
    if (strcmp(args, "list") == 0) {
        kprintf("thupkg - installed/known packages (design preview):\n");
        kprintf("  thu-coreutils   0.2.0   [designed]\n");
        kprintf("  thu-terminal    0.2.0   [designed]\n");
        kprintf("  thu-files       0.1.0   [planned]\n");
        kprintf("  thu-settings    0.1.0   [planned]\n");
        kprintf("Note: thupkg is a design preview; no real install backend yet.\n");
    } else {
        kprintf("thupkg <command>\n");
        kprintf("  list            List known packages\n");
        kprintf("  info <name>     (planned)\n");
        kprintf("  install <name>  (planned, sandboxed + signed)\n");
        kprintf("This is a design preview, not a working package manager.\n");
    }
}

static void cmd_color(const char *args) {
    int value = 0;
    bool any = false;
    for (const char *p = args; *p >= '0' && *p <= '9'; p++) {
        value = value * 10 + (*p - '0');
        any = true;
    }
    if (!any || value < 0 || value > 15) {
        kprintf("usage: color <0-15>\n");
        return;
    }
    vga_set_color((uint8_t)value, VGA_BLACK);
    kprintf("Text color set to %d.\n", value);
}

static void cmd_crash(const char *args) {
    if (strcmp(args, "div0") == 0) {
        kprintf("Triggering divide-by-zero to test the fault handler...\n");
        volatile int a = 1;
        volatile int b = 0;
        volatile int c = a / b;
        (void)c;
    } else {
        kprintf("usage: crash div0   (testing only - will fault the CPU)\n");
    }
}

static void reboot_machine(void) {
    kprintf("Rebooting THUOS...\n");
    uint8_t status = 0x02;
    while (status & 0x02) status = inb(0x64);
    outb(0x64, 0xFE);            /* pulse CPU reset line via 8042 */
    for (;;) __asm__ volatile("hlt");
}

static void halt_machine(void) {
    kprintf("System halted. It is now safe to close the emulator.\n");
    __asm__ volatile("cli");
    for (;;) __asm__ volatile("hlt");
}

static void execute(char *line) {
    /* Split into command and argument string. */
    char *args = line;
    while (*args && *args != ' ') args++;
    if (*args == ' ') { *args = '\0'; args++; while (*args == ' ') args++; }

    if (line[0] == '\0')                 return;
    else if (strcmp(line, "help") == 0)    cmd_help();
    else if (strcmp(line, "about") == 0)   cmd_about();
    else if (strcmp(line, "version") == 0) cmd_version();
    else if (strcmp(line, "status") == 0)  cmd_status();
    else if (strcmp(line, "sysinfo") == 0) cmd_sysinfo();
    else if (strcmp(line, "uptime") == 0)  cmd_uptime();
    else if (strcmp(line, "ticks") == 0)   kprintf("%u\n", pit_ticks());
    else if (strcmp(line, "mem") == 0)     cmd_mem();
    else if (strcmp(line, "echo") == 0)    kprintf("%s\n", args);
    else if (strcmp(line, "banner") == 0)  print_banner();
    else if (strcmp(line, "color") == 0)   cmd_color(args);
    else if (strcmp(line, "thupkg") == 0)  cmd_thupkg(args);
    else if (strcmp(line, "clear") == 0)   vga_clear();
    else if (strcmp(line, "crash") == 0)   cmd_crash(args);
    else if (strcmp(line, "reboot") == 0)  reboot_machine();
    else if (strcmp(line, "halt") == 0)    halt_machine();
    else kprintf("Unknown command: %s (try 'help')\n", line);
}

void shell_run(void) {
    char line[LINE_MAX];
    size_t len = 0;

    kprintf("Type 'help' to begin.\n\n");
    kprintf("thuos> ");

    for (;;) {
        char c = keyboard_getchar();

        if (c == '\n') {
            kputc('\n');
            line[len] = '\0';
            execute(line);
            len = 0;
            kprintf("thuos> ");
        } else if (c == '\b') {
            if (len > 0) { len--; kputc('\b'); }
        } else if (len < LINE_MAX - 1 && c >= ' ') {
            line[len++] = c;
            kputc(c);
        }
    }
}
