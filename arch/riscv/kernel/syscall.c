#include "syscall.h"
#include "sched.h"
#include "sysproc.h"
#include "put.h"
#include "vm.h"
#include "riscv.h"
#include "sysfile.h"
#include "sysother.h"


static uint64 (*syscalls[])(uintptr_t *) = {
    [SYS_clone]         sys_clone,
    [SYS_execve]        sys_execve,
    [SYS_mmap]          sys_mmap,
    [SYS_munmap]        sys_munmap,
	[SYS_uname]         sys_uname,
    [SYS_times]         sys_times,
    [SYS_gettimeofday]  sys_gettimeofday,
    [SYS_wait4]         sys_wait4,
    [SYS_exit]          sys_exit,
    [SYS_getppid]       sys_getppid,
    [SYS_getpid]        sys_getpid,

    [SYS_getcwd]        sys_getcwd,
    [SYS_dup]           sys_dup,
    [SYS_dup3]          sys_dup3,
    [SYS_chdir]         sys_chdir,
    [SYS_openat]        sys_openat,
    [SYS_getdents64]    sys_getdents64,
    [SYS_close]         sys_close,
    [SYS_read]          sys_read,
    [SYS_write]         sys_write,
    [SYS_unlinkat]      sys_unlinkat,
    [SYS_mkdirat]       sys_mkdirat,
    [SYS_fstat]         sys_fstat,
	[SYS_nanosleep]     sys_nanosleep,
	[SYS_sched_yield]   sys_sched_yield,
    [SYS_brk]           sys_brk,
    [SYS_pipe2]         sys_pipe2,
};

/**
 * @brief 进行系统调用
 * @param regs struct task的栈
 */
void syscall(uintptr_t *regs){
	int syscall_NR = regs[REG_A(7)];

    if(syscall_NR > 0 && syscalls[syscall_NR]) {
        regs[REG_A(0)]=syscalls[syscall_NR](regs);
    }
}
