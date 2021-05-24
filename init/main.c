#include "riscv.h"
#include "put.h"
#include "vm.h"
#include "timer.h"
#include "system.h"

volatile static int started = 0;

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
		timer_init();
		__sync_synchronize();
		started = 1;
	}
	else{
    	while (started == 0);
	}
	os_test();
}
