#include "types.h"
#include "riscv.h"
#include "sched.h"
#include "string.h"
#include "syscall.h"
#include "put.h"

void strap_handler(int64 scause, int64 sepc, uintptr_t *regs){
	memcpy(current->stack, regs, STACK_SIZE);
	if(scause>=0){//exception
		#ifdef DEBUG
		printf("[strap_handler]Get into EXP\n");
		#endif
		scause&=0xff;
		switch(scause){
			case U_ECALL:{
				syscall(regs);
				w_csr(sepc, sepc+4);
				break;
			}
			case INSTRUCTION_PAGE_FAULT:{
				printf("[S-Error!]Instruction page fault!\n");
				while(1);
			}
			case LOAD_PAGE_FAULT:{
				printf("[S-Error!]Load page fault!\n");
				while(1);
			}
			case STORE_PAGE_FAULT:{
				printf("[S-Error!]Store page fault!\n");
				while(1);
			}
		}
	}
	else{//interrupt
		scause&=0x3f;
		switch(scause){
			case S_TIME:do_timer();break;
		}
	}
}