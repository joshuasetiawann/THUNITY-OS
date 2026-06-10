/* THUOS — ring 3 entry/exit and TSS load (AT&T / GAS syntax).
 *
 * usermode_enter(entry, user_stack_top): from ring 0, build an interrupt-return
 *   frame for ring 3 and `iret` into `entry` running on `user_stack_top`. Before
 *   doing so it saves the callee-saved registers and the kernel esp so the
 *   matching exit path can unwind straight back here.
 *
 * usermode_resume_kernel(): called from the SYS_EXIT handler (already in ring 0,
 *   reached through int 0x80). It does NOT iret back to ring 3; instead it
 *   restores the saved kernel stack and returns as if usermode_enter() had
 *   returned normally to its caller. This is a deliberate one-way unwind — the
 *   user "process" is single-shot, so there is no context to preserve.
 */
.text

.global usermode_enter
.type usermode_enter, @function
usermode_enter:                       /* (void (*entry)(void), uint32_t ustack_top) */
    pushl   %ebp
    pushl   %ebx
    pushl   %esi
    pushl   %edi
    movl    %esp, g_um_kresume_esp     /* remember where to unwind back to */

    movl    20(%esp), %ecx             /* entry          (after 4 pushes)  */
    movl    24(%esp), %edx             /* user_stack_top                   */

    movl    $0x23, %eax                /* user data selector (idx 4, RPL 3) */
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs

    pushl   $0x23                      /* SS  = user data                  */
    pushl   %edx                       /* ESP = user stack top             */
    pushfl
    popl    %eax
    andl    $0xFFFFBFFF, %eax          /* clear NT (no task switch on iret) */
    orl     $0x00000200, %eax          /* set IF (interrupts on in ring 3)  */
    pushl   %eax                       /* EFLAGS                            */
    pushl   $0x1B                      /* CS  = user code (idx 3, RPL 3)    */
    pushl   %ecx                       /* EIP = entry                       */
    iret                               /* ---> ring 3                       */
.size usermode_enter, . - usermode_enter

.global usermode_resume_kernel
.type usermode_resume_kernel, @function
usermode_resume_kernel:
    cli
    movl    $0x10, %eax                /* kernel data selectors */
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movl    g_um_kresume_esp, %esp     /* abandon the ring-0 entry stack */
    popl    %edi
    popl    %esi
    popl    %ebx
    popl    %ebp
    ret                                /* return into usermode_enter's caller */
.size usermode_resume_kernel, . - usermode_resume_kernel

.global tss_flush
.type tss_flush, @function
tss_flush:                             /* (uint32_t selector) */
    movl    4(%esp), %eax
    ltr     %ax
    ret
.size tss_flush, . - tss_flush

.section .note.GNU-stack,"",@progbits
