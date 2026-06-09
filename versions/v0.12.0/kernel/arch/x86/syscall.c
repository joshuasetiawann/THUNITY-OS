/* THUOS — system call handlers + int 0x80 dispatch. See syscall.h. */
#include "syscall.h"
#include "syscall_core.h"
#include "isr.h"          /* registers_t */
#include "idt.h"
#include "kprintf.h"
#include "pit.h"

static syscall_fn table[SYS_MAX];

/* --- handlers (ring 0 for now; pointers are kernel addresses) --- */
static int32_t sys_uptime(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c; return (int32_t)pit_ticks();
}
static int32_t sys_write(uint32_t fd, uint32_t buf, uint32_t len) {
    (void)fd;
    const char *p = (const char *)(uintptr_t)buf;
    for (uint32_t i = 0; i < len; i++) kputc(p[i]);
    return (int32_t)len;
}
static int32_t sys_getpid(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c; return 0;                 /* kernel task */
}
static int32_t sys_version(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c; return 0x000C;            /* 0.12 packed */
}

extern void syscall_stub(void);   /* syscall_stub.S */

void syscall_init(void) {
    syscall_table_init(table, SYS_MAX);
    syscall_register(table, SYS_MAX, SYS_UPTIME,  sys_uptime);
    syscall_register(table, SYS_MAX, SYS_WRITE,   sys_write);
    syscall_register(table, SYS_MAX, SYS_GETPID,  sys_getpid);
    syscall_register(table, SYS_MAX, SYS_VERSION, sys_version);
    /* vector 0x80, kernel code segment, present + DPL 3 + 32-bit interrupt gate */
    idt_set_gate(0x80, (uint32_t)syscall_stub, 0x08, 0xEE);
}

/* CS of the most recent int 0x80. For a ring-3 caller the CPU records the user
 * code selector (RPL 3) here, so (cs & 3) == 3 is hard proof the trap really
 * originated in user mode rather than being faked from the kernel. */
static volatile uint32_t last_cs = 0;

/* Called from syscall_stub.S with the saved register frame. The number is in
 * eax and args in ebx/ecx/edx; the result is written back to the saved eax,
 * which `popa` restores so the caller receives it in eax. */
void syscall_dispatch(registers_t *r) {
    last_cs = r->cs;
    r->eax = (uint32_t)syscall_call(table, SYS_MAX, r->eax, r->ebx, r->ecx, r->edx);
}

uint32_t syscall_last_cs(void) { return last_cs; }

int syscall_register_extra(int num, syscall_fn fn) {
    return syscall_register(table, SYS_MAX, num, fn);
}

int syscall_count(void) {
    int n = 0;
    for (int i = 0; i < SYS_MAX; i++) if (table[i]) n++;
    return n;
}

static uint32_t slen(const char *s) { uint32_t n = 0; while (s[n]) n++; return n; }

int32_t syscall_invoke(uint32_t num, uint32_t a, uint32_t b, uint32_t c) {
    int32_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a), "c"(b), "d"(c) : "memory");
    return ret;
}

void syscall_selftest(void) {
    const char *msg = "  [sys] write via int 0x80: hello from the syscall ABI\n";
    syscall_invoke(SYS_WRITE, 1, (uint32_t)(uintptr_t)msg, slen(msg));
    int32_t t = syscall_invoke(SYS_UPTIME, 0, 0, 0);
    kprintf("  [sys] int 0x80 SYS_UPTIME returned %d ticks\n", t);
}
