#include "sysproc.h"
#include "sched.h"
#include "vm.h"
#include "types.h"
#include "riscv.h"
#include "clone.h"
#include "exec.h"

/**
 * @brief 系统调用：创建一个子进程
 * @param regs struct task的栈
 * @return 成功返回子进程ID，失败返回-1
 */
long sys_clone(uintptr_t *regs)
{
    int flags=regs[REG_A(0)];
    void *stack=(void *)regs[REG_A(1)];
    pid_t *ptid=(pid_t *)regs[REG_A(2)];
    void *tls=(void *)regs[REG_A(3)];
    pid_t *ctid=(pid_t *)regs[REG_A(4)];
    return clone(flags, stack, ptid, tls, ctid);
}

/**
 * @brief 系统调用：执行一个指定的程序
 * @param regs struct task的栈
 * @return  成功不返回，失败返回-1
 */
long sys_execve(uintptr_t *regs)
{
    char *path=regs[REG_A(0)];
    char **argv=regs[REG_A(1)];
    char **envp=regs[REG_A(2)];
    long ret = exec(path, argv, envp);
    return ret;
}

/**
 * @brief 系统调用：等待进程改变状态
 * @param regs struct task
 * @return 成功返回进程ID，如果指定了WNOHANG，且进程还未改变状态，直接返回0；失败则返回-1
 */
long sys_wait4(uintptr_t *regs)
{
    long ret;
    long pid=regs[REG_A(0)];
    long* status=(long*)regs[REG_A(1)];
    long options=regs[REG_A(2)];
    ret=wait(pid, status, options);
    return ret;
}

/**
 * @brief 系统调用：触发进程终止
 * @param regs struct task
 */
long sys_exit(uintptr_t *regs)
{
    long status=regs[REG_A(0)];
    exit(status);
}

/**
 * @brief 系统调用：获得父进程ID
 * @param regs struct task
 * @return 成功则返回父进程ID
 */
long sys_getppid(uintptr_t *regs)
{
    return getppid();
}

/**
 * @brief 系统调用：获得进程ID
 * @param regs struct task
 * @return 成功则返回进程ID
 */
long sys_getpid(uintptr_t *regs)
{
    return getpid();
}

/**
 * @brief 系统调用：将文件或设备映射到内存中
 * @param regs struct task
 * @return 成功返回已映射区域的指针，失败返回-1
 */
long sys_mmap(uintptr_t *regs)
{
    void *start=(void *)regs[REG_A(0)];
    size_t len=regs[REG_A(1)];
    int prot=regs[REG_A(2)];
    int flags=regs[REG_A(3)];
    int fd=regs[REG_A(4)];
    off_t off=regs[REG_A(5)];
    return mmap(start, len, prot, flags, fd, off);
}

/**
 * @brief 系统调用：将文件或设备取消映射到内存中
 * @param regs struct task
 * @return 成功返回0，失败返回-1
 */
long sys_munmap(uintptr_t *regs)
{
    void *start=(void *)regs[REG_A(0)];
    size_t len=regs[REG_A(1)];
    return munmap(start, len);
}

/**
 * @brief 系统调用：让出调度器
 * @param regs struct task
 * @return 成功返回0，失败返回-1
 */
long sys_sched_yield(uintptr_t *regs)
{
	yield();
	return 0;
}

/**
 * @brief 系统调用：执行线程睡眠
 * @param regs struct task
 * @return 成功返回0，失败返回-1
 */
long sys_nanosleep(uintptr_t *regs)
{
	struct timespec *req=(struct timespec *)regs[REG_A(0)];
	struct timespec *rem=(struct timespec *)regs[REG_A(1)];
	nanosleep(req, rem);
	return 0;
}

/**
 * @brief 系统调用：修改数据段大小
 * @param regs struct task
 * @return 成功返回0，失败返回-1
 */
long sys_brk(uintptr_t *regs)
{
    int64 addr=regs[REG_A(0)];
    int ret=brk(addr);
    if(ret==-1) return -1;
    else return 0;
}