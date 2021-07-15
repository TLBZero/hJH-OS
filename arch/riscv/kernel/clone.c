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
#include "clone.h"

void forket(){
	ret_from_fork(current->stack);
}

/**
 * 复制一个进程
 */
pid_t clone(int flag, void *stack, pid_t* ptid, void *tls, pid_t* ctid)
{
	int child=alloc_pid();
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

	task[child]->mm->pagetable=(pagetable_t)kmalloc(PAGE_SIZE);
	memcpy(task[child]->mm->pagetable, current->mm->pagetable, PAGE_SIZE);

	if (!stack)
		stack = kmalloc(PAGE_SIZE);
	else
		stack = (void *)kwalkaddr(current->mm->pagetable, (uint64)stack);
	
	delete_mapping(task[child]->mm->pagetable, (USER_END-PAGE_SIZE), PAGE_SIZE, 0);
	create_mapping(task[child]->mm->pagetable, (USER_END-PAGE_SIZE), (uint64)stack, PAGE_SIZE, PTE_R|PTE_W|PTE_U);
	memcpy(stack, (void*)(USER_END-PAGE_SIZE), PAGE_SIZE);

	task[child]->thread.sp=(uint64)task[child]+PAGE_SIZE;
	task[child]->thread.ra=(uint64)&forket;
	uint64 sepc;
	r_csr(sepc, sepc);
	task[child]->thread.sepc= (sepc + 4);

	procfile_init(task[child]);

	printf("[clone PID = %d] Process fork from [PID = %d] Successfully! counter = %d\n",task[child]->pid,current->pid,task[child]->counter);
	return task[child]->pid;
}