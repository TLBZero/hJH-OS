#include "time.h"
#include "sched.h"

extern struct task_struct* current;
extern struct task_struct* task[NR_TASKS];

/* 返回4个时间 */
long times(struct tms *t)
{
    uint64 time;
    r_csr(time, time);
	t->tms_cstime=current->cstime;
	t->tms_cutime=current->cutime;
	t->tms_stime=current->stime;
	t->tms_utime=current->utime;
    if(t->tms_cstime<0 || t->tms_cutime<0 || t->tms_stime<0 || t->tms_utime<0) return -1;
    return time;
}

int gettimeofday(struct timespec *t)
{
    uint64 time;
    r_csr(time, time);
    time*=1000;
    t->tv_sec=time/1000000000;
    t->tv_nsec=time%1000000000;
    if(t->tv_sec<0 || t->tv_nsec<0) return -1;
    return 0;
}