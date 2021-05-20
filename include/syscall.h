#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "types.h"

#define SYS_clone 220
#define SYS_execve 221
#define SYS_wait4 260
#define SYS_exit 93
#define SYS_getppid 173
#define SYS_getpid 172
#define SYS_getpid 172
#define SYS_mmap 222
#define SYS_munmap 215

void syscall(uintptr_t *regs);

#endif