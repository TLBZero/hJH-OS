#include "types.h"
#include "riscv.h"
#include "sched.h"
#include "string.h"
#include "syscall.h"
#include "put.h"
#include "plic.h"
#include "rustsbi.h"
#include "vm.h"

void dumpInfo(){
	uint64 sstatus, sepc, scause, sie, sip, stval, ra, sp;
	r_csr(sstatus, sstatus);
	r_csr(sepc, sepc);
	r_csr(scause, scause);
	r_csr(sie, sie);
	r_csr(sip, sip);
	r_csr(stval, stval);
	r_reg(ra, ra);
	r_reg(sp, sp);
	printf("[PID]%d\n", current->pid);
	printf("[csrInfo]\nsstatus:%p\nsepc:%p\nscause:%p\nsie:%p\nsip:%p\nstval:%p\n", sstatus, sepc, scause, sie, sip, stval);
	printf("[threadInfo]\nra:%p\nsp:%p\n", current->thread.sp, current->thread.ra);
	printf("[procInfo]\nra:%p, sp:%p\n", current->thread.sp, current->thread.ra);
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
				//sleep(&tmp, &current->lk);
				w_csr(sepc, sepc+4);
				syscall(regs);
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
		if(scause == S_TIME)
			do_timer(sstatus);
		#ifdef QEMU 
		if (scause == 9) 
		#else 
		int stval;
		r_csr(stval, stval);
		if (scause == 1 && stval == 9) 
		#endif
		{
			int irq = plic_claim();
			if (UART_IRQ == irq) {
				// keyboard input, disabled
				printf("keboard\n");
				int c = console_getchar();
				if (-1 != c) {
					consoleintr(c);
				}
			}
			else if (DISK_IRQ == irq) {
				printf("disk\n");
				disk_intr();
			}
			else if (irq) {
				printf("unexpected interrupt irq = %d\n", irq);
			}

			if (irq) plic_complete(irq);

			#ifndef QEMU 
			int64 sip;
			r_csr(sip, sip);
			w_csr(sip, sip&~2);	// Clear pending bit
			set_mie();
			#endif
		}
	}
}