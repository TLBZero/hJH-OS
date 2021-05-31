#ifndef _SYSOTHER_H_
#define _SYSOTHER_H_

#include "types.h"

/* syscall in time & systemInfo */
long sys_uname(uintptr_t *regs);
long sys_times(uintptr_t *regs);
long sys_gettimeofday(uintptr_t *regs);

#endif