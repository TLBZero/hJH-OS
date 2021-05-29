#include "sched.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "types.h"
#include "disk.h"
#include "put.h"
#include "vm.h"
#include "string.h"
#include "timer.h"
#include "pipe.h"
#include "time.h"
#include "systemInfo.h"
#include "sysfile.h"

// #define DEBUG
struct task_struct* current;
struct task_struct* task[NR_TASKS];

long sec,nsec,flag;  //nanosleep所需参数

extern void __switch_to(unsigned long long, unsigned long long);
extern void first_switch_to();
extern void idle();

void task_init(void){
	task[0]=current=(struct task_struct*)TASK_VM_START;		//Task0 Base
	task[0]->state=TASK_READY;
	task[0]->counter=1;
	task[0]->priority=5;
	task[0]->blocked=0;
	task[0]->pid=0;
	task[0]->ppid=0;
	task[0]->stack = (uint64)kmalloc(sizeof(STACK_SIZE));
	task[0]->xstate=-1;
	task[0]->chan=0;

	/* 新增-wdl */
	task[0]->utime=0;
    task[0]->stime=0;
    task[0]->cutime=0;
    task[0]->cstime=0;

	initlock(&(task[0]->lk), "proc");
	task[0]->thread.sp=TASK_VM_START+TASK_SIZE;	//Task0 Base + 4kb

	/* FS */
	procfile_init(task[0]);

	for(int i=1;i<=LAB_TEST_NUM;i++){//init task[i]
		task[i]=(struct task_struct*)(TASK_VM_START+i*TASK_SIZE);
		task[i]->state=TASK_READY;
		task[i]->counter=rand();
		task[i]->priority=5;
		task[i]->blocked=0;
		task[i]->pid=i;
		task[i]->ppid=0;

		task[i]->stack = (uint64)kmalloc(STACK_SIZE);
		task[i]->allocated_stack = 0;

		/* 新增-wdl */
		task[i]->xstate=-1;
		task[i]->chan=0;
		task[i]->utime=0;
    	task[i]->stime=0;
    	task[i]->cutime=0;
    	task[i]->cstime=0;
		initlock(&(task[i]->lk), "proc");

		task[i]->mm = (struct mm_struct*)kmalloc(sizeof(struct mm_struct));
		task[i]->mm->pagetable_address = K_VA2PA((uint64)kmalloc(PAGE_SIZE));
		memcpy(task[i]->mm->pagetable_address, kernel_pagetable, PAGE_SIZE);

		// do_mmap(task[i]->mm, 0, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC);
		// do_mmap(task[i]->mm, USER_END-PAGE_SIZE, PAGE_SIZE, PROT_READ|PROT_WRITE);

		// create_mapping(task[i]->mm->pagetable_address, 0, dead_loop, TASK_SIZE, PTE_R|PTE_W|PTE_X|PTE_U);
		// create_mapping(task[i]->mm->pagetable_address, USER_END-PAGE_SIZE*i,kmalloc(PAGE_SIZE),PAGE_SIZE,PTE_R|PTE_W|PTE_U);

		task[i]->sscratch = (void*)task[i]+PAGE_SIZE;
		task[i]->xstate=-1;
		task[i]->chan=0;
		initlock(&(task[i]->lk), "proc");
		task[i]->thread.sp=(unsigned long long)task[i]+TASK_SIZE;
		task[i]->thread.ra=(unsigned long long)&first_switch_to;

		procfile_init(task[i]);

		printf("[PID = %d] Process Create Successfully! counter = %d priority = %d\n",task[i]->pid, task[i]->counter, task[i]->priority);
	}
}

void do_timer(int64 sstatus){
	timer_tick();

	/* 新增-wdl */
	time(sstatus);

#ifdef SJF
	printf("[PID = %d] Context Calculation: counter = %d\n",current->pid, current->counter);
	if(!current->pid||current->counter<=0||--current->counter<=0){
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

	i=NR_TASKS;
	c=-1;
	next=0;
	prio=-1;
	p=&task[NR_TASKS];
#ifdef SJF
	while(--i){
		if(!*(--p)) continue;
		if((*p)->state==TASK_READY&&(*p)->counter>0){
			if(c<0||c>(*p)->counter) c=(*p)->counter,next=i;//if p is the first non-empty task or p's counter is smaller, then choose it.
		}
	}
	for(int i=1;i<=LAB_TEST_NUM;i++) if(task[i] && task[i]->state==TASK_READY&&task[i]->counter==0) {
		task[i]->counter=rand();
		printf("[PID = %d] Reset counter = %d\n",task[i]->pid, task[i]->counter);
	}

#endif
#ifdef PRIORITY
	while(--i){
		if(!*(--p)) continue;
		if((*p)->state==TASK_READY&&(*p)->counter>0){
			if(c<0||prio>(*p)->priority) c=(*p)->counter,prio=(*p)->priority,next=i;//if p is the first non-empty one or p is prior than the last one, then choose it.
			else if(prio==(*p)->priority){//else if p'priority equals the last one, compary the counter and choose the smaller one.
				if(c>(*p)->counter) c=(*p)->counter,next=i;
			}
		}
	}
	if(c>0)break;
#endif
	if(current!=task[next]) printf("[!] Switch from task %d [task struct: %p, sp: %p] to task %d [task struct: %p, sp: %p], prio: %d, counter: %d\n",current->pid,(uint64)current,current->thread.sp,task[next]->pid,(uint64)task[next],task[next]->thread.sp,task[next]->priority, task[next]->counter);

#ifdef PRIORITY
	printf("task's priority changed\n");
	for(int i=1;i<=LAB_TEST_NUM;i++){
		task[i]->priority=rand();
		printf("[PID = %d] counter = %d priority = %d\n",task[i]->pid, task[i]->counter, task[i]->priority);
	}
#endif
	
	//spipe_test();
	switch_to(task[next]);
}

void switch_to(struct task_struct* next){
	if(current==next) return;
	__switch_to(&current, &next);
}

void task_test(void){
	// fat_init();
	// btest();
	while(1);
}

uint64 alloc_pid()
{
	// static uint64 currentPID = LAB_TEST_NUM;
	// return ++currentPID;
	for (int i=1; i<NR_TASKS; i++)
		if (task[i]==NULL)
			return i;
}

void forket(){
	ret_from_fork(current->stack);
}

pid_t clone(int flag, void *stack, pid_t ptid, void *tls, pid_t ctid)
{
	uint64 child=alloc_pid();
	if (child < 0) return -1;
	task[child]=(struct task_struct*)kmalloc(PAGE_SIZE);
	task[child]->state=TASK_READY;
	task[child]->counter=rand();
	task[child]->priority=5;
	task[child]->blocked=0;
	task[child]->pid=child;
	task[child]->ppid=current->pid;
	if (stack!=NULL)
		task[child]->stack=stack;
	else
		task[child]->stack=(uint64*)kmalloc(STACK_SIZE);
	memcpy(task[child]->stack, current->stack, STACK_SIZE);
	task[child]->stack[REG_A(0)] = 0;
	task[child]->stack[SEPC] += 4;

	task[child]->mm=(struct mm_struct*)kmalloc(sizeof(struct mm_struct));
	struct vm_area_struct *parentVMA=current->mm->vma, *childVMA;
	if (parentVMA) do {
		childVMA=(struct vm_area_struct*)kmalloc(sizeof(struct vm_area_struct));
		memcpy(childVMA, parentVMA, sizeof(struct vm_area_struct));
		if(task[child]->mm->vma) {
			childVMA->vm_prev=task[child]->mm->vma;
			childVMA->vm_next=task[child]->mm->vma->vm_next;
			task[child]->mm->vma->vm_next->vm_prev=childVMA;
			task[child]->mm->vma->vm_next=childVMA;
		}
		else {
			task[child]->mm->vma=childVMA;
			childVMA->vm_prev=childVMA;
			childVMA->vm_next=childVMA;
		}
		parentVMA=parentVMA->vm_next;
	}while(parentVMA!=current->mm->vma);

	task[child]->mm->pagetable_address=K_VA2PA((uint64)kmalloc(PAGE_SIZE));
	memcpy(task[child]->mm->pagetable_address, kernel_pagetable, PAGE_SIZE);

	task[child]->sscratch=task[child]->stack[SSCRATCH];
	task[child]->thread.sp=(void*)task[child]+PAGE_SIZE;
	task[child]->allocated_stack = K_VA2PA((uint64)kmalloc(PAGE_SIZE));
	memcpy((void*)task[child]->allocated_stack, (void*)(USER_END-PAGE_SIZE), PAGE_SIZE);

	task[child]->thread.ra=(unsigned long long)&forket;

	printf("[PID = %d] Process fork from [PID = %d] Successfully! counter = %d\n",task[child]->pid,current->pid,task[child]->counter);
	return task[child]->pid;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copy(int user, void* dst, void *src, uint64 len){
	if(user){
	// return copyout(p->pagetable, dst, src, len);
	// return copyout2(dst, src, len);
	} else {
		memcpy(dst, src, len);
		return 0;
	}
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
	acquire(&(current->lk));
	if(current->pid==0){
		//task[0]不可以结束
		return;
	}

	//如果某个进程的父进程ppid==当前进程的pid，则把他的父进程变为task[0]
	for(int i=1;i<NR_TASKS;i++)
	{
		if(!task[i] || i==current->pid) continue;
		//acquire(&(task[i]->lk));
		if(task[i]->ppid==current->pid) task[i]->ppid=0;
		//release(&(task[i]->lk));
	}

	//若当前进程的父进程处于睡眠状态，则唤醒
	//acquire(&(task[current->ppid]->lk));
	if(task[current->ppid]->state==TASK_SLEEPING) task[current->ppid]->state=TASK_READY;
	//release(&(task[current->ppid]->lk));

	current->xstate=status;
	current->state=TASK_ZOMBIE;
	release(&(current->lk));
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
		//acquire(&(task[i]->lk));
		if(task[i]->state==TASK_SLEEPING && task[i]->chan==chan)
		{
			task[i]->state=TASK_READY;
		}
		//release(&(task[i]->lk));
	}
}


/* 等待某一/任意子进程改变状态 */
long wait(long pid, long* status, long options)
{
	int i;
	if (options == WCONTINUED) return -1;
	acquire(&(current->lk));
	if(pid==-1)
	{
	repeat1:
		//task[0]是第一个创建的进程，且不会exit，所以他不可能是其他任意进程的子进程
		for(i=1;i<NR_TASKS;i++)
		{
			if(!task[i] || i==current->pid) continue;
			acquire(&(task[i]->lk));
			if(task[i]==0){
				release(&(task[i]->lk));
				continue;
			}
			if(task[i]->ppid==current->pid && task[i]->state==TASK_ZOMBIE) 
			{
				release(&(task[i]->lk));
				break;
			}
			release(&(task[i]->lk));
		}
		//没有任何的子进程的状态是zombie
		if(i>=NR_TASKS) 
		{
			if(options==WNOHANG) 
			{
				release(&(current->lk));
				return 0;
			}
			else if(options==WUNTRACED)
			{
				current->state=TASK_SLEEPING;
				release(&(current->lk));
				schedule();
				acquire(&(current->lk));
				goto repeat1;
			}
		}

		current->cutime += task[i]->utime;
		current->cstime += task[i]->stime;
		int pid=task[i]->pid;
		*status=task[i]->xstate;
		//测试的时候没有加kfree
		kfree(task[i]);
		task[i]=0;
		release(&(current->lk));
		return pid;
	}
	else
	{
	repeat2:
		acquire(&(task[pid]->lk));
		if(task[pid]==0) 
		{
			release(&(task[pid]->lk));
			release(&(current->lk));
			return -1;
		}
		if(task[pid]->ppid!=current->pid) 
		{
			release(&(current->lk));
			return -1;
		}
		if(task[pid]->state!=TASK_ZOMBIE) 
		{
			if(options==WNOHANG) 
			{
				release(&(task[pid]->lk));
				release(&(current->lk));
				return 0;
			}
			else if(options==WUNTRACED)
			{
				current->state=TASK_SLEEPING;
				release(&(task[pid]->lk));
				release(&(current->lk));
				schedule();
				acquire(&(current->lk));
				goto repeat2;
			}
		}
		else 
		{
			current->cutime += task[pid]->utime;
			current->cstime += task[pid]->stime;
			*status=task[pid]->xstate;
			//测试的时候没有加kfree
			kfree(task[pid]);
			task[pid]=0;
			release(&(current->lk));
			return pid;
		}
	}
	release(&(current->lk));
	return -1;
}

/* 让出调度器 */
void yield()
{
	schedule();
}

/* 线程睡眠 */
void nanosleep(struct timespec *req, struct timespec *rem)
{
	flag=current->pid;
	sec=task[0]->stime+req->tv_sec;
	nsec=task[0]->utime+req->tv_nsec;
	current->state=TASK_SLEEPING;
	if(rem!=0){
		rem->tv_sec=0;
		rem->tv_nsec=0;
	}
	schedule();
}

void time(int64 sstatus)
{
	sstatus&=0x100;
	task[0]->stime++;     //秒
	task[0]->utime+=10;   //纳秒
	if(task[0]->utime==1000000000) 
	{
		task[0]->stime++;
		task[0]->utime=0;
	}
	if(sstatus) current->stime++;
	else current->utime++;

	if(flag!=0)
	{
		if(task[0]->stime > sec || sec==task[0]->stime && task[0]->utime >=nsec)
		{
			task[flag]->state=TASK_READY;
			flag=0;
		}
	}
}
