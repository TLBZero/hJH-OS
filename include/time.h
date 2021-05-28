#ifndef _TIMES_H
#define _TIMES_H

#include "sched.h"
#include "types.h"

struct tms {
	long tms_utime;
	long tms_stime;
	long tms_cutime;
	long tms_cstime;
};

struct timespec {
	time_t tv_sec;        /* 秒 */
	long   tv_nsec;       /* 纳秒, 范围在0~999999999 */
};

/* 返回4个时间 */
long times(struct tms *);

/* 获取时间 */
int gettimeofday(struct timespec *);

#endif
