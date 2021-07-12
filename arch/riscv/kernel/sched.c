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
struct task_struct* current;
struct task_struct* nextTask;
struct task_struct* task[NR_TASKS];

long sec,nsec,flag;  // nanosleep所需参数
static int fs_init = 0;

extern void __switch_to(unsigned long long, unsigned long long);
extern void idle();
void first_switch_to();
extern uint64 kwalkaddr(pagetable_t kpt, uint64 va);

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
	task[0]->stack = (uint64)kmalloc(sizeof(STACK_SIZE));
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
	task[0]->thread.sscratch = (void*)task[0]+PAGE_SIZE;

	task[0]->mm = (struct mm_struct*)kmalloc(sizeof(struct mm_struct));
	task[0]->mm->pagetable = (uint64)kmalloc(PAGE_SIZE);
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

	task[pid]->stack = (uint64)kmalloc(STACK_SIZE);
	/* 新增-wdl */
	task[pid]->xstate=-1;
	task[pid]->chan=0;
	task[pid]->utime=0;
	task[pid]->stime=0;
	task[pid]->cutime=0;
	task[pid]->cstime=0;
	initlock(&(task[pid]->lk), "proc");

	task[pid]->mm = (struct mm_struct*)kmalloc(sizeof(struct mm_struct));
	task[pid]->mm->pagetable = (uint64)kmalloc(PAGE_SIZE);
	memset(task[pid]->mm->pagetable, 0, PAGE_SIZE);
	memcpy(task[pid]->mm->pagetable, kernel_pagetable, PAGE_SIZE);

	do_mmap(task[pid]->mm, USER_END-PAGE_SIZE, PAGE_SIZE, PROT_READ|PROT_WRITE);
	create_mapping(task[pid]->mm->pagetable, USER_END-PAGE_SIZE, (uint64)kmalloc(PAGE_SIZE),PAGE_SIZE,PTE_R|PTE_W|PTE_U);

	task[pid]->xstate=-1;
	task[pid]->chan=0;
	initlock(&(task[pid]->lk), "proc");
	task[pid]->thread.sp=(unsigned long long)USER_END;
	task[pid]->thread.ra=(unsigned long long)&first_switch_to;
	task[pid]->thread.sscratch = (void*)task[pid]+PAGE_SIZE;

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
	int i, c, next,prio;
	struct task_struct **p;

	i=NR_TASKS;
	c=-1;
	next=0;
	prio=-1;
	p=&task[NR_TASKS];
	/* SJF */
	for(int i=1;i<NR_TASKS;i++) if(task[i] && task[i]->state==TASK_READY&&task[i]->counter==0) {
		task[i]->counter=rand();
		printf("[PID = %d] Reset counter = %d\n",task[i]->pid, task[i]->counter);
	}
	while(--i){
		if(!*(--p)) continue;
		if((*p)->state==TASK_READY&&(*p)->counter>0){
			if(c<0||c>(*p)->counter) c=(*p)->counter,next=i;//if p is the first non-empty task or p's counter is smaller, then choose it.
		}
	}

	if(current!=task[next]) printf("[!] Switch from task %d to task %d, prio: %d, counter: %d\n",current->pid,task[next]->pid,task[next]->priority, task[next]->counter);

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
	__switch_to(&current, &nextTask);
}

void first_switch_to(){
	printf("first_switch_to\n");
	if(!fs_init){
		fs_init = 1;
		fat_init();
	}
	asm("li t0, 0x100;\
		 csrc sstatus, t0;\
		 li t0, 0x40002;\
		 csrs sstatus, t0;\
		 csrw sepc, x0;\
		 sret");
}


void task_test(){
	// uint64 _a0 = 1;
	// uint64 _a1 = 0;
	// uint64 _a2 = 0;
	// uint64 _a3 = 0;
	// uint64 _a4 = 0;
	// register uint64 a0 asm("a0") = _a0;
	// register uint64 a1 asm("a1") = _a1;
	// register uint64 a2 asm("a2") = _a2;
	// register uint64 a3 asm("a2") = _a3;
	// register uint64 a4 asm("a2") = _a4;
	// register long syscall_id asm("a7") = 64;
	// char a[7];
	// a[0]='h';
	// a[1]='e';
	// a[2]='l';
	// a[3]='l';
	// a[4]='o';
	// a[5]='\0';
	// a[6]='\0';
	// a0 = 1;
	// a1 = a;
	// a2 = 8;
	// a3 = _a3;
	// a4 = _a4;
	// syscall_id = 64;
	// asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(syscall_id));
	// fat_init();
	// btest();
	// test_sdcard();
	while(1);
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

int alloc_pid()
{
	for (int i=1; i<NR_TASKS; i++)
		if (task[i]==NULL)
			return i;
	return -1;
}

void forket(){
	ret_from_fork(current->stack);
}

/**
 * 复制一个进程
 */
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
	task[child]->stack=(uint64*)kmalloc(STACK_SIZE);
	task[child]->xstate=-1;
	task[child]->chan=0;

	task[child]->utime=0;
	task[child]->stime=0;
	task[child]->cutime=0;
	task[child]->cstime=0;
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

	task[child]->mm->pagetable=(uint64)kmalloc(PAGE_SIZE);
	memcpy(task[child]->mm->pagetable, current->mm->pagetable, PAGE_SIZE);

	if (!stack)
		stack = (uint64)kmalloc(PAGE_SIZE);
	else
		stack = kwalkaddr(current->mm->pagetable, stack);
	delete_mapping(task[child]->mm->pagetable, (USER_END-PAGE_SIZE), PAGE_SIZE);
	create_mapping(task[child]->mm->pagetable, (USER_END-PAGE_SIZE), stack, PAGE_SIZE, PTE_R|PTE_W|PTE_U);
	memcpy(stack, (void*)(USER_END-PAGE_SIZE), PAGE_SIZE);

	task[child]->thread.sp=(void*)task[child]+PAGE_SIZE;
	task[child]->thread.ra=(unsigned long long)&forket;
	uint64 sepc;
	r_csr(sepc, sepc);
	task[child]->thread.sepc= (sepc + 4);

	procfile_init(task[child]);

	printf("[clone PID = %d] Process fork from [PID = %d] Successfully! counter = %d\n",task[child]->pid,current->pid,task[child]->counter);
	return task[child]->pid;
}

int loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint filesz) 
{
	uint64 pa;
	uint n;
	if ( (va%PAGE_SIZE) != 0)
		panic("loadseg error, va should be aligned!");

	for (uint i=0; i<filesz; i+=PAGE_SIZE)
	{
		pa = kwalkaddr(pagetable, va);
		if (pa == NULL)
			panic("loadseg error, address wrong!");
		if (filesz - i < PAGE_SIZE)
			n = filesz-1;
		else
			n = PAGE_SIZE;
		if (eread(ip, pa, offset+i, n)!=n)
			return -1;
	}

	return 0;
}

int parse_ph_flags(struct proghdr* ph)
{
	int flag = 0;
	if (ph->type != ELF_PROG_LOAD)
		flag = 1;
	if (ph->memsz < ph->filesz)
		flag = 1;
	if (ph->vaddr + ph->memsz < ph->vaddr)
		flag = 1;
	return flag;
}

int exec(const char *path, char *const argv[], char *const envp[])
{
	char _path[FAT32_MAX_PATH];
	strcpy(_path, path);
	struct dirent* inode;
	struct elfhdr elf;
	struct proghdr ph;
	pagetable_t pagetable=NULL, oldpagetable;
	int i, off;

	if ((pagetable = (pagetable_t)kmalloc(PAGE_SIZE)) == NULL)
	{
		return -1;
	}
	memset(pagetable, 0, PAGE_SIZE);
	memcpy((void *)pagetable, (void *)kernel_pagetable, PAGE_SIZE);
	if ((inode = ename(_path)) == NULL)
		goto fail;
	if (eread(inode, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
		goto fail;
	if (elf.magic != ELF_MAGIC)
		goto fail;
	//load program
	for (i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph))
	{
		if ((eread(inode, (uint64)&ph, off, sizeof(ph))) != sizeof(ph))
			goto fail;
		if (parse_ph_flags(&ph)==1)
			goto fail;
		uint64 va = kmalloc(ph.memsz);

		create_mapping(pagetable, ph.vaddr, (uint64)kmalloc(ph.memsz), ph.memsz, (ph.flags)|PTE_U|PTE_X);
		if (ph.vaddr % PAGE_SIZE != 0)
			goto fail;
		if (loadseg(pagetable, ph.vaddr, inode, ph.off, ph.filesz) < 0)
			goto fail;
	}
	create_mapping(pagetable, USER_END-PAGE_SIZE, (uint64)kmalloc(PAGE_SIZE),PAGE_SIZE,PTE_R|PTE_W|PTE_U);
	
	c_csr(sstatus, 0x100);
	s_csr(sstatus, 0x40002);
	w_csr(sscratch, USER_END);
	current->mm->pagetable = pagetable;
	current->thread.sscratch = USER_END;
	current->thread.sepc = elf.entry;
	w_csr(sepc, elf.entry);
	asm volatile("csrw satp, %0"::"r"(SV39|((uint64)pagetable>>12)));
	asm volatile("sfence.vma");
	return 0;

fail:
	#ifdef DEBUG
	printf("[S] exec failed!\n");
	#endif
	kfree(pagetable);
	return -1;
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
int growtask(uint size){
	acquire(&current->lk);
	int oldsz = current->size;
	int newsz = current->size + size;
	if(newsz > oldsz){
		for(newsz = PAGE_ROUNDUP(newsz);oldsz<newsz;oldsz+=PAGE_SIZE){
			void *mem = (uint64)kmalloc(PAGE_SIZE);
			if(!mem) return -1; // alloc fail
			create_mapping(current->mm->pagetable, oldsz, mem, PAGE_SIZE, PTE_R|PTE_W|PTE_U);
		}
	}else if(newsz < oldsz){
		for(newsz = PAGE_ROUNDUP(newsz);oldsz>newsz;oldsz-=PAGE_SIZE){
			kfree(oldsz-PAGE_SIZE);
			delete_mapping(current->mm->pagetable, oldsz-PAGE_SIZE, PAGE_SIZE);
		}
	}
	release(&current->lk);
	return 0;
}

long brk(int64 addr) {
	if (addr <= 0) return current->size;
	if(growtask(addr - current->size)==-1) return -1;
	return current->size;
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
		if(time/1000000000 > sec || sec==time/1000000000 && time%1000000000 >=nsec)
		{
			task[flag]->state=TASK_READY;
			flag=0;
		}
	}
}

void taskFree(int i)
{
	for(int j=0;j<PROCOFILENUM;j++)
	{
		frelease(task[i]->FTable[j]);
	}
	eput(task[i]->cwd);
	kfree(task[i]->stack);
	kfree(task[i]->mm->vma);
	kfree(task[i]->mm);
	kfree(task[i]);
	task[i]=0;
}