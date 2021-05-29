#include "time.h"
#include "sched.h"

extern struct task_struct* current;
extern struct task_struct* task[NR_TASKS];

/* 返回4个时间 */
long times(struct tms *t)
{
	t->tms_cstime=current->cstime;
	t->tms_cutime=current->cutime;
	t->tms_stime=current->stime;
	t->tms_utime=current->utime;
    if(t->tms_cstime<0 || t->tms_cutime<0 || t->tms_stime<0 || t->tms_utime<0) return -1;
    return task[0]->stime;
}

int gettimeofday(struct timespec *t)
{
    t->tv_sec=task[0]->stime;
    t->tv_nsec=task[0]->utime;
    if(t->tv_sec<0 || t->tv_nsec<0) return -1;
    return 0;
}