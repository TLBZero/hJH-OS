#include "riscv.h"
#include "put.h"
#include "vm.h"
#include "timer.h"
#include "system.h"
#include "disk.h"
#include "plic.h"
#include "sched.h"
#include "fat32.h"
#include "sysfile.h"
#include "console.h"
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
		console_init();
		printf_init();
		print_logo();
		paging_init();
		plic_init();
		plic_inithart();
		task_init();
		#ifndef QEMU
		fpioa_pin_init();
		dmac_init();
		#endif 
		disk_init();
		binit();
		fat_init();
		sysfile_init();
		timer_init();
		__sync_synchronize();
		started = 1;
		asm volatile("csrw satp, %0"::"r"(SV39|((uint64)current->mm->pagetable>>12)));
		asm volatile("sfence.vma");
		idle();
	}
	else{
    	while (started == 0);
	}
}
