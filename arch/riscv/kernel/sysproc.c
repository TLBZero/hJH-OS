#include "sysproc.h"
#include "sched.h"
#include "vm.h"

uint64 sys_clone(int flags, void *stack, pid_t *ptid, void *tls, pid_t *ctid)
{
    return clone(flags, stack, ptid, tls, ctid);
}

uint64 sys_execve()
{

}

uint64 sys_wait4()
{

}

uint64 sys_mmap(void *start, size_t len, int prot, int flags,
                  int fd, off_t off)
{
    return mmap(start, len, prot, flags, fd, off);
}

uint64 sys_munmap(void *start, size_t len)
{
    return munmap(start, len);
}