#include "riscv.h"
#include "put.h"
#include "vm.h"
#include "timer.h"
#include "system.h"
#include "disk.h"
#include "plic.h"
#include "sched.h"
#include "fat32.h"
volatile static int started = 0;
extern void idle();
static inline void inithartid(unsigned long hartid) {
	asm volatile("mv tp, %0" : : "r" (hartid & 0x1));
}

void start_kernel(unsigned long hartid)
{
	inithartid(hartid);
	if(hartid == 0){
		env_init();
		printf_init();
		print_logo();
		paging_init();
		task_init();
		plic_init();
		plic_inithart();
		disk_init();
		binit();
		fat_init();
		fat_test();
		timer_init();
		__sync_synchronize();
		started = 1;
		idle();
	}
	else{
    	while (started == 0);
	}
}
