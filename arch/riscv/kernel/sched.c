#include "sched.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "types.h"
#include "sdcard.h"
#include "fpioa.h"
#include "dmac.h"

struct task_struct* current;
struct task_struct* task[NR_TASKS];
//struct sleeplock lk;

extern void __switch_to(unsigned long long, unsigned long long);
extern void first_switch_to();

void task_init(void){
	//initsleeplock(&lk, "wdl");
	puts("task init...\n");
	task[0]=current=(struct task_struct*)TASK_VM_START;		//Task0 Base
	task[0]->state=TASK_RUNNING;
	task[0]->counter=0;
	task[0]->priority=5;
	task[0]->blocked=0;
	task[0]->pid=0;
	task[0]->ppid=0;
	task[0]->xstate=-1;
	task[0]->chan=0;
	initlock(&(task[0]->lk), "proc");
	task[0]->thread.sp=TASK_VM_START+TASK_SIZE;	//Task0 Base + 4kb

	for(int i=1;i<=LAB_TEST_NUM;i++){//init task[i]
		task[i]=(struct task_struct*)(TASK_VM_START+i*TASK_SIZE);
		task[i]->state=TASK_RUNNING;
#ifdef SJF
		task[i]->counter=rand();
#endif
#ifdef PRIORITY
		task[i]->counter=(8-i)>0?8-i:0;
#endif
		task[i]->priority=5;
		task[i]->blocked=0;
		task[i]->pid=i;
		task[i]->ppid=0;
		task[i]->xstate=-1;
		task[i]->chan=0;
		initlock(&(task[i]->lk), "proc");
		task[i]->thread.sp=(unsigned long long)task[i]+TASK_SIZE;
		task[i]->thread.ra=(unsigned long long)&first_switch_to;
#ifdef SJF
		printf("[PID = %d] Process Create Successfully! counter = %d\n",task[i]->pid, task[i]->counter);
#endif
#ifdef PRIORITY
		printf("[PID = %d] Process Create Successfully! counter = %d priority = %d\n",task[i]->pid, task[i]->counter, task[i]->priority);
#endif
	}
}

void do_timer(void){
#ifdef SJF
	printf("[PID = %d] Context Calculation: counter = %d\n",current->pid, current->counter);
	if(current->counter<=0||--current->counter<=0){
		schedule();
	}
#endif
#ifdef PRIORITY
	if(current->pid!=0&&(current->counter<=0||--current->counter<=0)){
		task[current->pid]->counter=(8-current->pid>0)?8-current->pid:0;//if process ended, initialize it again
		printf("[PID = %d] Reset counter = %d, priority = %d\n", current->pid, current->counter, current->priority);
	}
	schedule();
#endif
}

void schedule(void){
	int i, c, next,prio;
	struct task_struct **p;

	while(1){
		i=NR_TASKS;
		c=-1;
		next=0;
		prio=-1;
		p=&task[NR_TASKS];
#ifdef SJF
		while(--i){
			if(!*(--p)) continue;
			if((*p)->state==TASK_RUNNING&&(*p)->counter>0){
				if(c<0||c>(*p)->counter) c=(*p)->counter,next=i;//if p is the first non-empty task or p's counter is smaller, then choose it.
			}
		}
		if(c>0) break;
		for(int i=1;i<=LAB_TEST_NUM;i++) if(task[i]->state==TASK_RUNNING) {
			task[i]->counter=rand();
			printf("[PID = %d] Reset counter = %d\n",task[i]->pid, task[i]->counter);
		}
#endif
#ifdef PRIORITY
		while(--i){
			if(!*(--p)) continue;
			if((*p)->state==TASK_RUNNING&&(*p)->counter>0){
				if(c<0||prio>(*p)->priority) c=(*p)->counter,prio=(*p)->priority,next=i;//if p is the first non-empty one or p is prior than the last one, then choose it.
				else if(prio==(*p)->priority){//else if p'priority equals the last one, compary the counter and choose the smaller one.
					if(c>(*p)->counter) c=(*p)->counter,next=i;
				}
			}
		}
		if(c>0)break;
#endif
	}
	if(current!=task[next]) printf("[!] Switch from task %d [task struct: %p, sp: %p] to task %d [task struct: %p, sp: %p], prio: %d, counter: %d\n",current->pid,(uint64)current,current->thread.sp,task[next]->pid,(uint64)task[next],task[next]->thread.sp,task[next]->priority, task[next]->counter);

#ifdef PRIORITY
	puts("task's priority changed\n");
	for(int i=1;i<=LAB_TEST_NUM;i++){
		task[i]->priority=rand();
		printf("[PID = %d] counter = %d priority = %d\n",task[i]->pid, task[i]->counter, task[i]->priority);
	}
#endif

	//acquiresleep(&lk);
	//printf("%s current pid=%d\n",lk.name, lk.owner);
	//releasesleep(&lk);

	switch_to(task[next]);
}

void switch_to(struct task_struct* next){
	if(current==next) return;
	__switch_to(&current, &next);
}

/*  */
void dead_loop(void){
	fpioa_pin_init();
	dmac_init();
	sdcard_init();
	test_sdcard();
	while(1);
}
/* 返回当前进程的pid*/
long getpid(void){
	return current->pid;
}

/* 返回当前进程的ppid*/
long getppid(void){
	return current->ppid;
}

/* 退出 */
void exit(long status){
	if(current->pid==0){
		//task[0]不可以结束
		return;
	}

	//如果某个进程的父进程ppid==当前进程的pid，则把他的父进程变为task[0]
	for(int i=1;i<NR_TASKS;i++)
	{
		if(task[i]->ppid==current->pid) task[i]->ppid=0;
	}

	//若当前进程的父进程处于睡眠状态，则唤醒
	if(task[current->ppid]->state==TASK_SLEEPING) task[current->ppid]->state=TASK_RUNNING;

	current->xstate=status;
	current->state=TASK_ZOMBIE;
	schedule();
}

void sleep(void *chan, struct spinlock *lk)
{
	if(lk!=&current->lk)
	{
		acquire(&(current->lk));
		release(lk);
	}
	current->chan=chan;
	current->state=TASK_SLEEPING;
	schedule();
	current->chan=0;
	if(lk!=&current->lk)
	{
		release(&(current->lk));
		acquire(lk);
	}
}

void wakeup(void *chan)
{
	for(int i=1;i<NR_TASKS;i++)
	{
		if(task[i]==0) continue;
		acquire(&(task[i]->lk));
		if(task[i]->state==TASK_SLEEPING && task[i]->chan==chan)
		{
			task[i]->state=TASK_RUNNING;
		}
		release(&(task[i]->lk));
	}
}
