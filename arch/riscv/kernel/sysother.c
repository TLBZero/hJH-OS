#include "systemInfo.h"
#include "time.h"
#include "types.h"
#include "sysother.h"
#include "sched.h"
#include "riscv.h"

/**
 * @brief 系统调用：返回系统信息
 * @param regs struct task的栈
 * @return 成功返回0，否则返回-1
 */
long sys_uname(uintptr_t *regs)
{
    struct utsname *uts= (struct utsname *)regs[REG_A(0)];
    uname(uts);
    return 0;
}

/**
 * @brief 系统调用：返回4个时间
 * @param regs struct task的栈
 * @return 返回系统开始运行的时间
 */
long sys_times(uintptr_t *regs)
{
    long  time;
    struct tms *t= (struct tms *)regs[REG_A(0)];
    time=times(t);
    if(time<0) return -1;
    return time;
}

/**
 * @brief 系统调用：返回系统开始运行的时间
 * @param regs struct task的栈
 * @return 成功返回0，否则返回-1
 */
long sys_gettimeofday(uintptr_t *regs)
{
    int ret;
    struct timespec *t= (struct timespec *)regs[REG_A(0)];
    ret=gettimeofday(t);
    return ret;
}
