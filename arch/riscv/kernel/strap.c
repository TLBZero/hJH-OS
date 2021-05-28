#include "types.h"
#include "riscv.h"
#include "sched.h"
#include "string.h"
#include "syscall.h"
#include "put.h"
#include "plic.h"
#include "rustsbi.h"

static void dumpInfo(){
	uint64 sstatus, sepc, scause, sie, sip;
	r_csr(sstatus, sstatus);
	r_csr(sepc, sepc);
	r_csr(scause, scause);
	r_csr(sie, sie);
	r_csr(sip, sip);
	printf("[csrInfo]\nsstatus:%p\nsepc:%p\nscause:%p\nsie:%p\nsip:%p\n", sstatus, sepc, scause, sie, sip);
	printf("[threadInfo]ra:%p\nsp:%p\n", current->thread.sp, current->thread.ra);
}
void strap_handler(int64 scause, int64 sepc, uintptr_t *regs, int64 sstatus){
	memcpy(current->stack, regs, STACK_SIZE);
	// dumpInfo();
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
				dumpInfo();
				while(1);
			}
			case LOAD_PAGE_FAULT:{
				printf("[S-Error!]Load page fault!\n");
				dumpInfo();
				while(1);
			}
			case STORE_PAGE_FAULT:{
				printf("[S-Error!]Store page fault!\n");
				dumpInfo();
				while(1);
			}
		}
	}
	else{//interrupt
		scause&=0x3f;
		switch(scause){
			case S_TIME:do_timer(sstatus);break;
			case 9:{
					int irq = plic_claim();
					if (UART_IRQ == irq) {
						// keyboard input, disabled
						printf("keyboard interrupt, not done yet!\n");
					}
					else if (DISK_IRQ == irq) {
						disk_intr();
					}
					else if (irq) {
						printf("unexpected interrupt irq = %d\n", irq);
					}

					if (irq) plic_complete(irq);

					#ifndef QEMU 
					w_sip(r_sip() & ~2);    // clear pending bit
					sbi_set_mie();
					#endif
					break;
			}
		}
	}
}