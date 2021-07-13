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
#include "fat32.h"
#include "elf.h"
#include "sysfile.h"
#include "rand.h"
struct task_struct* current;
struct task_struct* nextTask;
struct task_struct* task[NR_TASKS];

long sec,nsec,flag;  // nanosleep所需参数
static int fs_init = 0;

extern void __switch_to(uint64, uint64);
extern void idle();
extern void ret_from_fork(uint64*);
extern uint64 kwalkaddr(pagetable_t kpt, uint64 va);

void first_switch_to();
/**
 * @brief 初始化进程0
 */
void task_init(void){
	task[0]=current=(struct task_struct*)TASK_START;		//Task0 Base
	task[0]->state=TASK_READY;
	task[0]->counter=1;
	task[0]->priority=5;
	task[0]->blocked=0;
	task[0]->pid=0;
	task[0]->ppid=0;
	task[0]->stack = (uint64*)kmalloc(sizeof(STACK_SIZE));
	task[0]->xstate=-1;
	task[0]->chan=0;
	task[0]->size=0x1000;

	/* 新增-wdl */
	task[0]->utime=0;
    task[0]->stime=0;
    task[0]->cutime=0;
    task[0]->cstime=0;

	initlock(&(task[0]->lk), "proc");
	task[0]->thread.sp=USER_END;
	task[0]->thread.sscratch = (uint64)task[0]+PAGE_SIZE;

	task[0]->mm = (struct mm_struct*)kmalloc(sizeof(struct mm_struct));
	task[0]->mm->pagetable = (pagetable_t)kmalloc(PAGE_SIZE);
	memcpy(task[0]->mm->pagetable, kernel_pagetable, PAGE_SIZE);

	uvmap(task[0], idle, PAGE_SIZE, 0);
	create_mapping(task[0]->mm->pagetable, (USER_END-PAGE_SIZE), (uint64)kmalloc(PAGE_SIZE), PAGE_SIZE, PTE_R|PTE_W|PTE_U);

	/* FS */
	procfile_init(task[0]);

	asm volatile("csrw satp, %0"::"r"(SV39|((uint64)current->mm->pagetable>>12)));
	asm volatile("sfence.vma");
}

/**
 * @brief 新建一个进程，其父进程是0
 * 
 * @return 新建成功则返回PCB，否则返回NULL
 */
struct task_struct* taskAlloc(){
	int pid = alloc_pid();
	if(pid<=0) return NULL;

	task[pid]=(struct task_struct*)(TASK_START+pid*TASK_SIZE);
	task[pid]->state=TASK_READY;
	task[pid]->counter=rand();
	task[pid]->priority=5;
	task[pid]->blocked=0;
	task[pid]->pid=pid;
	task[pid]->ppid=0;

	task[pid]->stack = (uint64*)kmalloc(STACK_SIZE);
	/* 新增-wdl */
	task[pid]->xstate=-1;
	task[pid]->chan=0;
	task[pid]->utime=0;
	task[pid]->stime=0;
	task[pid]->cutime=0;
	task[pid]->cstime=0;
	initlock(&(task[pid]->lk), "proc");

	task[pid]->mm = (struct mm_struct*)kmalloc(sizeof(struct mm_struct));
	task[pid]->mm->pagetable = (pagetable_t)kmalloc(PAGE_SIZE);
	memset(task[pid]->mm->pagetable, 0, PAGE_SIZE);
	memcpy(task[pid]->mm->pagetable, kernel_pagetable, PAGE_SIZE);

	do_mmap(task[pid]->mm, (void*)(USER_END-PAGE_SIZE), PAGE_SIZE, PROT_READ|PROT_WRITE);
	create_mapping(task[pid]->mm->pagetable, USER_END-PAGE_SIZE, (uint64)kmalloc(PAGE_SIZE),PAGE_SIZE,PTE_R|PTE_W|PTE_U);

	task[pid]->xstate=-1;
	task[pid]->chan=0;
	initlock(&(task[pid]->lk), "proc");
	task[pid]->thread.sp=(uint64)USER_END;
	task[pid]->thread.ra=(uint64)&first_switch_to;
	task[pid]->thread.sscratch = (uint64)task[pid]+PAGE_SIZE;

	procfile_init(task[pid]);

	printf("[PID = %d] Process Create Successfully! counter = %d priority = %d\n",task[pid]->pid, task[pid]->counter, task[pid]->priority);
	return task[pid];
}

/**
 * @brief 时钟中断
 * 
 */
void do_timer(int64 sstatus){
	timer_tick();

	/* 新增-wdl */
	time(sstatus);

	printf("[PID = %d] Context Calculation: counter = %d\n",current->pid, current->counter);
	if(!current->pid||current->counter<=0||--current->counter<=0){
		schedule();
	}
}

/**
 * @brief 作为scheduler
 * 
 */
void schedule(void){
	int i, c, next;
	struct task_struct **p;

	i=NR_TASKS;
	c=-1;
	next=0;
	p=&task[NR_TASKS];
	/* SJF */
	for(int i=1;i<NR_TASKS;i++) if(task[i] && task[i]->state==TASK_READY&&task[i]->counter==0) {
		task[i]->counter=rand();
		printf("[PID = %d] Reset counter = %d\n",task[i]->pid, task[i]->counter);
	}
	while(--i){
		if(!*(--p)) continue;
		if((*p)->state==TASK_READY&&(*p)->counter>0){
			if(c<0||c>(*p)->counter) c=(*p)->counter, next=i;
			//if p is the first non-empty task or p's counter is smaller, then choose it.
		}
	}

	if(current!=task[next])
		printf("[!] Switch from task %d to task %d, prio: %d, counter: %d\n", \
		current->pid,task[next]->pid,task[next]->priority, task[next]->counter);

	nextTask = task[next];
	switch_to();
}

/**
 * @brief 切换到nextTask指定的地方
 * 
 */
void switch_to(){
	if(current==nextTask) return;

	asm volatile("csrw satp, %0"::"r"(SV39|((uint64)nextTask->mm->pagetable>>12)));
	asm volatile("sfence.vma");
	__switch_to((uint64)&current, (uint64)&nextTask);
}

void first_switch_to(){
	printf("first_switch_to\n");
	if(!fs_init){
		fs_init = 1;
		fat_init();
	}
	asm("li t0, 0x100;     \
		 csrc sstatus, t0; \
		 li t0, 0x40002;   \
		 csrs sstatus, t0; \
		 csrw sepc, x0;    \
		 sret");
}

void task_test(){
	char a[8];
	a[0] = 'e';
	a[1] = 'x';
	a[2] = 'e';
	a[3] = 't';
	a[4] = 'e';
	a[5] = 's';
	a[6] = 't';
	a[7] = '\0';
	register uint64 a0 asm("a0") = 0;
	register uint64 a1 asm("a1") = 0;
	register uint64 a2 asm("a2") = 0;
	register uint64 a3 asm("a3") = 0;
	register uint64 a4 asm("a4") = 0;
	register uint64 syscall_id asm("a7") = 220;
	asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(syscall_id));
	a0 = a;
	syscall_id = 221;
	asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(syscall_id));
	// while(1);
}

/**
 * @brief 初始化用户态
 */
void user_init(){
	struct task_struct* task = taskAlloc();
	if(!task) panic("[user_init]user init fail!");

	uvmap(task, task_test, PAGE_SIZE, 0);
	return;
}

/**
 * @brief Copy to either a user address, or kernel address,
 * 
 * @param user Whether a user address
 * @return int 0 for success, -1 for failure
 */
int either_copy(int user, void* dst, void *src, uint64 len){
	memcpy(dst, src, len);
	return 0;
}

/* 申请新的pid*/
int alloc_pid()
{
	for (int i=1; i<NR_TASKS; i++)
		if (task[i]==NULL)
			return i;
	return -1;
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
	#ifdef DEBUG
	printf("sleep pid:%d\n", current->pid);
	#endif
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
			#ifdef DEBUG
			printf("wake pid:%d\n", task[i]->pid);
			#endif
			task[i]->state=TASK_READY;
		}
		//release(&(task[i]->lk));
	}
}

/**
 * @brief 按照size扩展进程的内存大小
 * 
 * @return 修改成功则返回新的size，否则返回-1 
 */
int growtask(int64 size){
	acquire(&current->lk);
	int64 oldsz = current->size;
	int64 newsz = current->size + size;
	if(newsz > oldsz){
		for(newsz=PAGE_ROUNDUP(newsz); oldsz<newsz; oldsz+=PAGE_SIZE){
			void *mem = kmalloc(PAGE_SIZE);
			if(!mem) return -1; // alloc fail
			create_mapping(current->mm->pagetable, oldsz, (uint64)mem, PAGE_SIZE, PTE_R|PTE_W|PTE_U);
		}
	}else if(newsz < oldsz){
		for(newsz = PAGE_ROUNDUP(newsz); oldsz>newsz; oldsz-=PAGE_SIZE){
			kfree((void*)(oldsz-PAGE_SIZE));
			delete_mapping(current->mm->pagetable, oldsz-PAGE_SIZE, PAGE_SIZE);
		}
	}
	release(&current->lk);
	return 0;
}

long brk(int64 addr) {
	if (addr <= 0) return current->size;
	if(growtask(addr - current->size)==-1)
		return -1;
	return current->size;
}

/* 等待某一或任意子进程改变状态 */
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
		taskFree(i);
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
			taskFree(pid);
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
	uint64 time;
    r_csr(time, time);
	time*=1000;
	sec=time/1000000000+req->tv_sec;
	nsec=time%1000000000+req->tv_nsec;
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
	uint64 time;
    r_csr(time, time);
	time*=1000;
	if(sstatus) current->stime++;
	else current->utime++;
	
	if(flag!=0)
	{
		if(time/1000000000 > sec || (sec==time/1000000000 && time%1000000000 >=nsec))
		{
			task[flag]->state=TASK_READY;
			flag=0;
		}
	}
}

/**
 * @brief Free a task struct
 * 
 * @param pid pid of target struct
 */
void taskFree(int pid)
{
	for(int j=0;j<PROCOFILENUM;j++)
	{
		frelease(task[pid]->FTable[j]);
	}
	kfree(task[pid]->stack);


	task[pid] = 0;
}