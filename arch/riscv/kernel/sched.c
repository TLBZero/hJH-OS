#include "riscv.h"
#include "sched.h"
#include "put.h"
#include "vm.h"

#ifdef SJF
    #define COUNTER(N) (N ? rand() : 0)
#endif
#ifdef PRIORITY
    #define COUNTER(N) (N ? 8 - N: 0)
#endif

struct task_struct* current;
struct task_struct* task[NR_TASKS];

extern void __switch_to(unsigned long long, unsigned long long);
extern void first_switch_to();

void task_init(void){
	puts("task init...\n");
	for(int i=0;i<=LAB_TEST_NUM;i++){//init task[i]
		task[i]=(struct task_struct*)kmalloc(PAGE_SIZE);
		task[i]->state=TASK_RUNNING;
		task[i]->counter = COUNTER(i);
		task[i]->priority=5;
		task[i]->blocked=0;
		task[i]->pid=i;
		task[i]->thread.sp=(unsigned long long)task[i]+TASK_SIZE;
		task[i]->thread.ra=(unsigned long long)&first_switch_to;
#ifdef SJF
		printf("[PID = %d] Process Create Successfully! counter = %d\n",task[i]->pid, task[i]->counter);
#endif
#ifdef PRIORITY
		printf("[PID = %d] Process Create Successfully! counter = %d priority = %d\n",task[i]->pid, task[i]->counter, task[i]->priority);
#endif
	}
	current = task[0];
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
		for(int i=1;i<=LAB_TEST_NUM;i++){
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
	switch_to(task[next]);
}

void switch_to(struct task_struct* next){
    if(current == next) return;
    __switch_to(&current, &next);
}

void dead_loop(void){
	while(1);
}
