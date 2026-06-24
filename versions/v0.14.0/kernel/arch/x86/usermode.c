/* THUOS — user mode (ring 3) kernel glue. See usermode.h. */
#include "usermode.h"
#include "usermode_core.h"
#include "syscall.h"
#include "tss.h"
#include "kprintf.h"

/* usermode.S */
extern void usermode_enter(void (*entry)(void), uint32_t user_stack_top);
extern void usermode_resume_kernel(void) __attribute__((noreturn));

/* Saved kernel esp for the one-way unwind out of ring 3. Read/written by the
 * assembly in usermode.S, so it must have external linkage and that exact name. */
uint32_t g_um_kresume_esp;

static int32_t g_um_exit_code;

/* The ring-3 stack. Lives in .bss inside the identity-mapped, now user-readable
 * low memory, so CPL 3 may use it. */
static uint8_t um_ustack[8192] __attribute__((aligned(16)));

/* int 0x80 from C, used by the ring-3 task. Inlined so the user task contains
 * only the trap instruction and no call into privileged kernel code. */
static inline int32_t u_syscall(uint32_t n, uint32_t a, uint32_t b, uint32_t c) {
    int32_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory");
    return ret;
}

static const char um_hello[] = "  [ring3] hello from CPL 3 - kernel reached only via int 0x80\n";

/* Runs at ring 3 (CPL 3). It cannot touch hardware or kernel-only memory
 * directly; the only way back into the kernel is the syscall gate. */
static void user_entry(void) {
    uint32_t len = 0;
    while (um_hello[len]) len++;
    u_syscall(SYS_WRITE,  1, (uint32_t)(uintptr_t)um_hello, len);
    u_syscall(SYS_GETPID, 0, 0, 0);
    u_syscall(SYS_EXIT,   0, 0, 0);                 /* unwinds back to ring 0 */
    for (;;) u_syscall(SYS_GETPID, 0, 0, 0);        /* unreached */
}

/* SYS_EXIT handler (ring 0). Unlike every other syscall it never returns through
 * the int 0x80 iret path; it unwinds straight back to usermode_run's caller. */
static int32_t sys_exit(uint32_t code, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    g_um_exit_code = (int32_t)code;
    usermode_resume_kernel();                       /* noreturn */
    return 0;                                        /* unreached */
}

void usermode_init(void) {
    syscall_register_extra(SYS_EXIT, sys_exit);
}

int usermode_run(void) {
    uint32_t top = (uint32_t)(uintptr_t)(um_ustack + sizeof um_ustack);
    top &= ~0xFu;                                   /* 16-byte align the user esp */
    g_um_exit_code = 0;
    usermode_enter(user_entry, top);                /* -> ring 3 ... SYS_EXIT -> here */
    return g_um_exit_code;
}

void usermode_selftest(void) {
    int code = usermode_run();
    uint32_t cs = syscall_last_cs();
    kprintf("  [user] back in ring 0 (exit %d); last int 0x80 came from CS=0x%02x => CPL %u\n",
            code, cs, cs & 3u);
}
