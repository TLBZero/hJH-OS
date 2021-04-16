#include "riscv.h"

void strap_handler(int64 scause, int64 sepc){
	if(scause>=0){//exception
		scause&=0xff;
		switch(scause){
			case INSTRUCTION_PAGE_FAULT:{
				kprintf("[S-Error!]Instruction page fault!\n");
				while(1);
			}
			case LOAD_PAGE_FAULT:{
				kprintf("[S-Error!]Load page fault!\n");
				while(1);
			}
			case STORE_PAGE_FAULT:{
				kprintf("[S-Error!]Store page fault!\n");
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