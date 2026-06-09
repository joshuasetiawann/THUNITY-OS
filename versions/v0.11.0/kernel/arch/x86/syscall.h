/* THUOS — system call interface (int 0x80).
 * A small, stable syscall ABI: number in eax, args in ebx/ecx/edx, return in
 * eax. This is the foundation for userspace; for now it is exercised from the
 * kernel (ring 0), and the same int 0x80 gate (DPL 3) will serve ring-3
 * processes once they exist. */
#ifndef THUOS_SYSCALL_H
#define THUOS_SYSCALL_H

#include "types.h"

#define SYS_UPTIME  0   /* () -> ticks                     */
#define SYS_WRITE   1   /* (fd, buf, len) -> bytes written */
#define SYS_GETPID  2   /* () -> current task id           */
#define SYS_VERSION 3   /* () -> packed kernel version     */

void    syscall_init(void);                 /* install the int 0x80 gate + handlers */
void    syscall_selftest(void);             /* exercise int 0x80 at boot            */
int     syscall_count(void);                /* number of registered syscalls        */
int32_t syscall_invoke(uint32_t num, uint32_t a, uint32_t b, uint32_t c); /* int 0x80 from C */

#endif /* THUOS_SYSCALL_H */
