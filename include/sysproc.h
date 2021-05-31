#ifndef _SYSPROC_H_
#define _SYSPROC_H_

#include "types.h"

/* syscall in process */
long sys_clone(uintptr_t *regs);
long sys_execve(uintptr_t *regs);
long sys_wait4(uintptr_t *regs);
long sys_exit(uintptr_t *regs);
long sys_getppid(uintptr_t *regs);
long sys_getpid(uintptr_t *regs);
long sys_mmap(uintptr_t *regs);
long sys_munmap(uintptr_t *regs);
long sys_nanosleep(uintptr_t *regs);
long sys_sched_yield(uintptr_t *regs);
long sys_brk(uintptr_t *regs);

#endif
