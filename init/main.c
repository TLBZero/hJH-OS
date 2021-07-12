/*
 * @Author: Yinwhe
 * @Date: 2021-07-10 20:06:58
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-07-12 13:16:28
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */
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
#include "sdcard.h"

extern void idle();
extern void env_init();

volatile static int started = 0;
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
		sysfile_init();
		timer_init();
		user_init();
		__sync_synchronize();
		started = 1;
		idle();
	}
	else{
    	while (started == 0);
	}
}
