#include "spinlock.h"
#include "types.h"
#include "put.h"
#include "sched.h"

// 初始化 
void initlock(struct spinlock *lk, char *name)
{
	lk->lock=0;
	lk->name=name;
	lk->owner=-1;
}

// 获得锁
void acquire(struct spinlock *lk)
{
	//关闭时钟中断
	//intr_off()；
	if(holding(lk))
	{
		//puts("already have acquired\n");
		//intr_on();
		return;
	}
	
	//spin
	while(__sync_lock_test_and_set(&lk->lock, 1) != 0)
    	;
		
	__sync_synchronize();
	lk->owner=getpid();
	//puts("acqire sucessfully\n");
	//intr_on();
	return;
}

// 释放锁
void release(struct spinlock *lk)
{
	//intr_off()；
	if(!holding(lk))
	{
		//puts("already have released\n");
		//intr_on();
		return;
	}

	__sync_synchronize();

	__sync_lock_release(&lk->lock);
	
	lk->owner=-1;

	//puts("release sucessfully\n");
	//intr_on();
	return;
}

// 检查锁是否被当前进程占用（1为占用），必须关闭中断
int holding(struct spinlock *lk)
{
	return (lk->lock && lk->owner == getpid());
}
