#include "syscall.h"
#include "sched.h"
// #include "sysproc.h"
#include "put.h"
#include "vm.h"
#include "riscv.h"
#include "sysfile.h"

extern uint64 sys_clone();
extern uint64 sys_execve();
extern uint64 sys_wait4();
extern uint64 sys_mmap(void *start, size_t len, int prot, int flags,
                  int fd, off_t off);
extern uint64 sys_munmap(void *start, size_t len);
extern uint64 kwalkaddr(pagetable_t kpt, uint64 va);

static uint64 (*syscalls[])(void) = {
    [SYS_clone]      sys_clone,
    [SYS_execve]     sys_execve,
    [SYS_mmap]       sys_mmap,
    [SYS_munmap]     sys_munmap,
};

void syscall(uintptr_t *regs){
	int syscall_NR = regs[REG_A(7)];
    uint64 a0 = regs[REG_A(0)];
    uint64 a1 = regs[REG_A(1)];
    uint64 a2 = regs[REG_A(2)];
    uint64 a3 = regs[REG_A(3)];
    uint64 a4 = regs[REG_A(4)];
    uint64 a5 = regs[REG_A(5)];
    switch (syscall_NR)
    {
    case SYS_write:
        regs[REG_A(0)] = sys_write(a0, a1, a2);
        break;
    case SYS_clone:
        regs[REG_A(0)] = sys_clone(a0, a1, a2, a3, a4);
        break;
    case SYS_mmap:
        regs[REG_A(0)] = sys_mmap(a0, a1, a2, a3, a4, a5);
        break;
    case SYS_munmap:
        regs[REG_A(0)] = sys_munmap(a0, a1);
        break;
    case SYS_execve:
        regs[REG_A(0)] = sys_execve(a0, a1, a2);
        kwalkaddr(current->mm->pagetable, 0);
    default:
        break;
    }
	// if (syscall_NR > 0 && syscall_NR < (sizeof(syscalls)/sizeof(syscalls[0])) && syscalls[syscall_NR]) {
    //     // regs[REG_A(0)] = syscalls[syscall_NR]();
        
    // }
    // else {
    //     kprintf("unknown syscall: pid %d %s", current->pid, syscall_NR);
    //     regs[REG_A(0)] = -1;
    // }
}