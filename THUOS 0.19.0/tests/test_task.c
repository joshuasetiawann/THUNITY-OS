/* THUOS — host unit test for the task stack-frame setup (task.c).
 * Verifies the initial frame so the asm context switch (context.S) will land in
 * entry() with zeroed callee-saved registers. The asm switch itself is
 * boot-verified in QEMU by CI. Build & run: make test
 *
 * Note: the kernel is 32-bit, so t->esp (uint32_t) holds the full stack pointer.
 * On a 64-bit host it is the truncated form of a real pointer, so we compute the
 * real frame pointer directly here rather than dereferencing t->esp. */
#define THUOS_HOSTED_TEST
#include "../kernel/sched/task.c"

#include <stdio.h>
#include <assert.h>

static void dummy_entry(void) { }

int main(void) {
    static unsigned char stk[4096];
    ktask_t t;

    task_init(&t, stk, sizeof stk, dummy_entry);

    /* task_init places 5 words just below the 16-byte-aligned top of the stack. */
    uintptr_t top = ((uintptr_t)stk + sizeof stk) & ~(uintptr_t)15u;
    uint32_t *sp  = (uint32_t *)(top - 20u);

    /* esp -> ebp(0), edi(0), esi(0), ebx(0); then entry as the ret target */
    assert(sp[0] == 0 && sp[1] == 0 && sp[2] == 0 && sp[3] == 0);
    assert(sp[4] == (uint32_t)(uintptr_t)dummy_entry);

    /* stored esp matches that frame; struct populated; frame in-bounds */
    assert(t.esp == (uint32_t)(uintptr_t)sp);
    assert(t.stack == stk && t.stack_sz == sizeof stk);
    assert((unsigned char *)sp >= stk && (unsigned char *)sp + 20 <= stk + sizeof stk);

    printf("task setup test: OK\n");
    printf("  initial frame -> entry, zeroed callee regs, esp consistent\n");
    return 0;
}
