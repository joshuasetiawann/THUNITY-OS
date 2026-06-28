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
#include "pmm.h"
#include "kheap.h"
#include "vmm.h"
#include "sched.h"
#include "coop.h"
#include "fs.h"
#include "syscall.h"
#include "usermode.h"
#include "lfb.h"
#include "gconsole.h"
#include "desktop.h"

#define LINE_MAX 128

/* Parse a 32-bit hex value, accepting an optional 0x prefix. */
static bool parse_hex(const char *s, uint32_t *out) {
    uint32_t v = 0;
    bool any = false;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    for (; *s; s++) {
        char c = *s;
        uint32_t d;
        if (c >= '0' && c <= '9')      d = (uint32_t)(c - '0');
        else if (c >= 'a' && c <= 'f') d = (uint32_t)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = (uint32_t)(c - 'A' + 10);
        else return false;
        v = (v << 4) | d;
        any = true;
    }
    *out = v;
    return any;
}

/* Parse an unsigned decimal value. */
static bool parse_uint(const char *s, uint32_t *out) {
    uint32_t v = 0;
    bool any = false;
    for (; *s >= '0' && *s <= '9'; s++) { v = v * 10u + (uint32_t)(*s - '0'); any = true; }
    *out = v;
    return any;
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
    kprintf("  help       Show this help\n");
    kprintf("  about      About THUOS\n");
    kprintf("  version    Kernel name and version\n");
    kprintf("  status     Implemented / in-progress / planned\n");
    kprintf("  sysinfo    System summary\n");
    kprintf("  uptime     Uptime in ticks and seconds\n");
    kprintf("  ticks      Raw PIT tick counter\n");
    kprintf("  mem        Memory summary (physical memory manager)\n");
    kprintf("  memmap     Multiboot memory map\n");
    kprintf("  pages      Page-frame statistics\n");
    kprintf("  allocpage  Allocate one 4 KiB physical frame\n");
    kprintf("  freepage A Free a frame by physical address (hex)\n");
    kprintf("  heap       Kernel heap stats (kmalloc arena)\n");
    kprintf("  kmalloc N  Allocate N bytes from the kernel heap\n");
    kprintf("  kfree A    Free a kmalloc pointer (hex address)\n");
    kprintf("  vmm [A]    Paging info; translate virtual addr A (hex)\n");
    kprintf("  ps         List scheduler tasks and states\n");
    kprintf("  sched      Run the round-robin scheduler (live demo)\n");
    kprintf("  tasks      Run cooperative multitasking (3 tasks, live)\n");
    kprintf("  ls         List files in the RAM filesystem\n");
    kprintf("  cat F      Print file F\n");
    kprintf("  write F T  Write text T to file F\n");
    kprintf("  sys        Invoke syscalls via int 0x80 (demo)\n");
    kprintf("  user       Drop to ring 3 and round-trip a syscall (CPL 3 demo)\n");
    kprintf("  apps       calc | files | devices | about  (or click the dock)\n");
    kprintf("  gui        Repaint the THU Desktop\n");
    kprintf("  echo       Print the rest of the line\n");
    kprintf("  banner     Show the THUOS banner\n");
    kprintf("  color N    Set text color (0-15)\n");
    kprintf("  thupkg     Package manager (design preview)\n");
    kprintf("  clear      Clear the screen\n");
    kprintf("  crash X    Test fault handler (div0) - testing only\n");
    kprintf("  reboot     Reboot the machine\n");
    kprintf("  halt       Halt the CPU\n");
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
    kprintf("  [done]    Physical memory manager (Milestone 0.3)\n");
    kprintf("  [done]    Kernel heap kmalloc/kfree (Milestone 0.4)\n");
    kprintf("  [done]    Paging ENABLED (0.7, CR0.PG, boot-verified QEMU)\n");
    kprintf("  [done]    Cooperative multitasking (0.9, boot-verified)\n");
    kprintf("  [done]    RAM filesystem ls/cat/write (0.10)\n");
    kprintf("  [done]    Syscall interface int 0x80 (0.11)\n");
    kprintf("  [done]    User mode (ring 3): TSS + iret + syscall from CPL 3 (0.12)\n");
    kprintf("  [done]    THU Desktop: VGA graphics + graphical terminal (0.13)\n");
    kprintf("  [done]    Aurora: high-res 1024x768x32 truecolor desktop (0.14)\n");
    kprintf("  [done]    Apps: mouse + clickable dock, Calculator/Files/System (0.15)\n");
    kprintf("  [done]    Polish: pictogram dock icons, clock, refined desktop (0.16)\n");
    kprintf("  [plan]    Movable windows, per-process isolation, ELF userspace apps\n");
}

static void cmd_sysinfo(void) {
    kprintf("%s %s \"%s\"\n", THUOS_NAME, THUOS_VERSION, THUOS_CODENAME);
    kprintf("Arch    : %s\n", THUOS_ARCH);
    kprintf("Kernel  : %s\n", THUOS_KERNEL_NAME);
    kprintf("Timer   : PIT @ %u Hz\n", pit_frequency());
    kprintf("Uptime  : %u s (%u ticks)\n", pit_seconds(), pit_ticks());
    if (pmm_available()) {
        kprintf("Memory  : ~%u KiB usable, %u free frames (4 KiB)\n",
                pmm_usable_bytes() / 1024u, pmm_free_frames());
    }
    kprintf("Build   : %s %s\n", THUOS_BUILD_DATE, THUOS_BUILD_TIME);
}

static void cmd_uptime(void) {
    kprintf("Uptime: %u ticks, approx %u seconds (PIT @ %u Hz)\n",
            pit_ticks(), pit_seconds(), pit_frequency());
}

static void cmd_mem(void) {
    if (!pmm_available()) {
        kprintf("No Multiboot memory information was provided by the bootloader.\n");
        return;
    }
    kprintf("Memory hint: %u KiB low, %u KiB high (Multiboot)\n",
            pmm_mem_lower_kb(), pmm_mem_upper_kb());
    pmm_print_stats();
}

static void cmd_allocpage(void) {
    uint32_t addr = pmm_alloc_frame();
    if (addr == 0) {
        kprintf("allocpage: out of physical memory (no free frame)\n");
        return;
    }
    kprintf("Allocated frame at physical 0x%08x (free now: %u frames)\n",
            addr, pmm_free_frames());
    kprintf("Free it with: freepage 0x%08x\n", addr);
}

static void cmd_freepage(const char *args) {
    uint32_t addr;
    if (!parse_hex(args, &addr)) {
        kprintf("usage: freepage <physical-address-in-hex>\n");
        return;
    }
    int r = pmm_free_frame(addr);
    switch (r) {
        case 0:  kprintf("Freed frame at 0x%08x (free now: %u frames)\n",
                         addr & ~(PMM_PAGE_SIZE - 1), pmm_free_frames()); break;
        case -1: kprintf("freepage: 0x%08x is unaligned or out of range\n", addr); break;
        case -2: kprintf("freepage: 0x%08x is a protected region (refused)\n", addr); break;
        case -3: kprintf("freepage: 0x%08x was already free\n", addr); break;
        default: kprintf("freepage: error %d\n", r); break;
    }
}

static void cmd_heap(void) {
    kprintf("Kernel heap (fixed arena, pre-paging):\n");
    kprintf("  total : %u bytes\n", (uint32_t)kheap_total());
    kprintf("  used  : %u bytes\n", (uint32_t)kheap_used());
    kprintf("  free  : %u bytes\n", (uint32_t)kheap_free());
    kprintf("  blocks: %u\n",       (uint32_t)kheap_blocks());
    kprintf("  integrity: %s\n", kheap_check() ? "OK" : "CORRUPT");
}

static void cmd_kmalloc(const char *args) {
    uint32_t n;
    if (!parse_uint(args, &n) || n == 0) {
        kprintf("usage: kmalloc <bytes>\n");
        return;
    }
    void *p = kmalloc((size_t)n);
    if (!p) {
        kprintf("kmalloc(%u): out of heap memory (free: %u bytes)\n", n, (uint32_t)kheap_free());
        return;
    }
    kprintf("kmalloc(%u) = 0x%08x  (heap free now: %u bytes)\n",
            n, (uint32_t)(uintptr_t)p, (uint32_t)kheap_free());
    kprintf("Free it with: kfree 0x%08x\n", (uint32_t)(uintptr_t)p);
}

static void cmd_kfree(const char *args) {
    uint32_t addr;
    if (!parse_hex(args, &addr)) {
        kprintf("usage: kfree <hex-address from kmalloc>\n");
        return;
    }
    kfree((void *)(uintptr_t)addr);
    kprintf("kfree(0x%08x): heap free now %u bytes, integrity %s\n",
            addr, (uint32_t)kheap_free(), kheap_check() ? "OK" : "CORRUPT");
}

static void cmd_vmm(const char *args) {
    if (!vmm_is_ready()) {
        kprintf("vmm: page tables were not built\n");
        return;
    }
    uint32_t va;
    if (parse_hex(args, &va)) {
        uint32_t p = vmm_phys_of(va);
        if (p == 0xFFFFFFFFu) kprintf("vmm: va 0x%08x is NOT mapped\n", va);
        else                  kprintf("vmm: va 0x%08x -> pa 0x%08x\n", va, p);
        return;
    }
    kprintf("Virtual memory (paging): %s\n",
            vmm_is_enabled() ? "ENABLED (CR0.PG)" : "tables built, not enabled");
    kprintf("  page directory : phys 0x%08x\n", vmm_dir_phys());
    kprintf("  identity map   : low %u KiB built\n", vmm_mapped_bytes() / 1024u);
    kprintf("  va 0x00000000  -> pa 0x%08x\n", vmm_phys_of(0x00000000u));
    kprintf("  va 0x00100000  -> pa 0x%08x\n", vmm_phys_of(0x00100000u));
    kprintf("  va 0x00400000  -> pa 0x%08x\n", vmm_phys_of(0x00400000u));
    kprintf("  running under paging: %s (boot-verified in QEMU/CI)\n",
            vmm_is_enabled() ? "YES" : "no");
    kprintf("  usage: vmm <hex-va>  to translate an address\n");
}

static void cmd_ls(void) {
    int shown = 0, max = kfs_max_files();
    kprintf("Files in ramfs (%u bytes used):\n", (uint32_t)kfs_bytes_used());
    for (int i = 0; i < max; i++) {
        const char *nm = kfs_name(i);
        if (nm) { kprintf("  %s  (%u bytes)\n", nm, (uint32_t)kfs_size(i)); shown++; }
    }
    if (!shown) kprintf("  (no files)\n");
}

static void cmd_cat(const char *args) {
    if (args[0] == '\0') { kprintf("usage: cat <file>\n"); return; }
    uint32_t n;
    const uint8_t *d = kfs_read(args, &n);
    if (!d) { kprintf("cat: %s: not found\n", args); return; }
    for (uint32_t i = 0; i < n; i++) kputc((char)d[i]);
    if (n == 0 || d[n - 1] != '\n') kputc('\n');
}

static void cmd_write(const char *args) {
    const char *sp = args;
    while (*sp && *sp != ' ') sp++;
    if (*sp != ' ') { kprintf("usage: write <file> <text>\n"); return; }
    char name[32];
    int nl = 0;
    for (const char *q = args; q < sp && nl < 31; q++) name[nl++] = *q;
    name[nl] = '\0';
    const char *content = sp + 1;
    uint32_t len = 0; while (content[len]) len++;
    int w = kfs_write(name, content, len);
    if (w < 0) kprintf("write: failed (file table or arena full)\n");
    else        kprintf("wrote %d bytes to '%s'\n", w, name);
}

static void cmd_sys(const char *args) {
    (void)args;
    const char *m = "  [sys_write] hello via int 0x80\n";
    uint32_t len = 0; while (m[len]) len++;
    kprintf("Invoking syscalls live via int 0x80:\n");
    syscall_invoke(SYS_WRITE, 1, (uint32_t)(uintptr_t)m, len);
    kprintf("  SYS_UPTIME  = %d ticks\n", syscall_invoke(SYS_UPTIME, 0, 0, 0));
    kprintf("  SYS_GETPID  = %d\n",       syscall_invoke(SYS_GETPID, 0, 0, 0));
    kprintf("  SYS_VERSION = 0x%04x\n",   syscall_invoke(SYS_VERSION, 0, 0, 0));
    kprintf("  registered syscalls: %d\n", syscall_count());
}

static void cmd_user(const char *args) {
    (void)args;
    kprintf("Entering ring 3 (user mode), then returning via the syscall gate...\n");
    int code = usermode_run();
    uint32_t cs = syscall_last_cs();
    kprintf("Back in ring 0: exit=%d, last int 0x80 from CS=0x%02x => CPL %u\n",
            code, cs, cs & 3u);
    if ((cs & 3u) == 3u)
        kprintf("  ring 3 confirmed: the CPU recorded CPL 3 for that syscall.\n");
    kprintf("  (ring 3 can reach the kernel only through int 0x80.)\n");
}

static void cmd_gui(const char *args) {
    (void)args;
    if (!lfb_active()) { kprintf("gui: high-res framebuffer not available\n"); return; }
    desktop_draw();                 /* repaint chrome */
    desktop_open_app(APP_TERMINAL); /* back to a fresh terminal */
}

static void cmd_thupkg(const char *args) {
    if (strcmp(args, "list") == 0) {
        kprintf("thupkg - installed/known packages (design preview):\n");
        kprintf("  thu-coreutils   0.3.0   [designed]\n");
        kprintf("  thu-terminal    0.3.0   [designed]\n");
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

    if (line[0] == '\0')                     return;
    else if (strcmp(line, "help") == 0)      cmd_help();
    else if (strcmp(line, "about") == 0)     cmd_about();
    else if (strcmp(line, "version") == 0)   cmd_version();
    else if (strcmp(line, "status") == 0)    cmd_status();
    else if (strcmp(line, "sysinfo") == 0)   cmd_sysinfo();
    else if (strcmp(line, "uptime") == 0)    cmd_uptime();
    else if (strcmp(line, "ticks") == 0)     kprintf("%u\n", pit_ticks());
    else if (strcmp(line, "mem") == 0)       cmd_mem();
    else if (strcmp(line, "memmap") == 0)    pmm_print_mmap();
    else if (strcmp(line, "pages") == 0)     pmm_print_stats();
    else if (strcmp(line, "allocpage") == 0) cmd_allocpage();
    else if (strcmp(line, "freepage") == 0)  cmd_freepage(args);
    else if (strcmp(line, "heap") == 0)      cmd_heap();
    else if (strcmp(line, "kmalloc") == 0)   cmd_kmalloc(args);
    else if (strcmp(line, "kfree") == 0)     cmd_kfree(args);
    else if (strcmp(line, "vmm") == 0)       cmd_vmm(args);
    else if (strcmp(line, "ps") == 0)        sched_klist();
    else if (strcmp(line, "sched") == 0)     sched_kdemo();
    else if (strcmp(line, "tasks") == 0)     coop_run_demo();
    else if (strcmp(line, "ls") == 0)        cmd_ls();
    else if (strcmp(line, "cat") == 0)       cmd_cat(args);
    else if (strcmp(line, "write") == 0)     cmd_write(args);
    else if (strcmp(line, "sys") == 0)       cmd_sys(args);
    else if (strcmp(line, "user") == 0)      cmd_user(args);
    else if (strcmp(line, "gui") == 0)       cmd_gui(args);
    else if (strcmp(line, "calc") == 0)      desktop_open_app(APP_CALC);
    else if (strcmp(line, "files") == 0)     desktop_open_app(APP_FILES);
    else if (strcmp(line, "devices") == 0)   desktop_open_app(APP_SYSTEM);
    else if (strcmp(line, "about") == 0)     desktop_open_app(APP_ABOUT);
    else if (strcmp(line, "apps") == 0)      kprintf("Apps: calc, files, devices, about (or click the dock).\n");
    else if (strcmp(line, "echo") == 0)      kprintf("%s\n", args);
    else if (strcmp(line, "banner") == 0)    print_banner();
    else if (strcmp(line, "color") == 0)     cmd_color(args);
    else if (strcmp(line, "thupkg") == 0)    cmd_thupkg(args);
    else if (strcmp(line, "clear") == 0)     { if (gcon_active()) gcon_clear(); else vga_clear(); }
    else if (strcmp(line, "crash") == 0)     cmd_crash(args);
    else if (strcmp(line, "reboot") == 0)    reboot_machine();
    else if (strcmp(line, "halt") == 0)      halt_machine();
    else kprintf("Unknown command: %s (try 'help')\n", line);
}

/* Line-editing state, shared by the blocking shell_run and the event-loop-driven
 * shell_feed_char (used when the terminal is one app among several). */
static char   s_line[LINE_MAX];
static size_t s_len = 0;

static void shell_prompt(void) { kprintf("thuos> "); }

void shell_start(void) {
    s_len = 0;
    kprintf("Type 'help' to begin.\n\n");
    shell_prompt();
}

void shell_feed_char(char c) {
    if (c == '\n') {
        kputc('\n');
        s_line[s_len] = '\0';
        execute(s_line);
        s_len = 0;
        shell_prompt();
    } else if (c == '\b') {
        if (s_len > 0) { s_len--; kputc('\b'); }
    } else if (s_len < LINE_MAX - 1 && c >= ' ') {
        s_line[s_len++] = c;
        kputc(c);
    }
}

void shell_run(void) {            /* text-mode fallback: own the CPU, block on keys */
    shell_start();
    for (;;) shell_feed_char(keyboard_getchar());
}
