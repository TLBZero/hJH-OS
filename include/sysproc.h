#ifndef _SYSPROC_H_
#define _SYSPROC_H_

#include "types.h"

uint64 sys_fork();
uint64 sys_execve();
uint64 sys_wait4();
uint64 sys_mmap();
uint64 sys_munmap();

#endif