.include "macro.S"
.section .text.entry
.align 2
.global trap_s

.equ FRAMESIZE, 300

trap_s:
        save_all

		csrr a0, scause
        csrr a1, sepc
        mv a2, sp
		call strap_handler

        load_all

		li t0, 0x100
		csrs sstatus, t0
sret

/* this function do the context switch
 * &current is stored in a0, &next is stored in a1
 */
.global __switch_to
.type __Switch_to, @function
__switch_to:
        ld t0, 0(a0)    # t0 <- pointer to current task_struct
        ld t1, 0(a1)    # t1 <- pointer to next task_struct

        addi t0, t0, THREAD_OFFSET      # t0 <- current thread struct
        addi t1, t1, THREAD_OFFSET      # t1 <- next thread struct

		csrr t2, sepc
		sd t2, 0(t0)	# store sepc
		addi t0, t0, 8

		csrr t2, sscratch
		sd t2, 0(t0)	# store sscratch
		addi t0, t0, 8

        save_tss t0

		ld t2, 0(t1)	# load sepc
		csrw sepc, t2
		addi t1, t1, 8
		
		ld t2, 0(t1)	# load sscratch
		csrw sscratch, t2
		addi t1, t1, 8

        load_tss t1

        # current <- next
        ld      t0, 0(a1) 
        sd      t0, 0(a0)
ret

.global first_switch_to
.type first_switch_to, @function
first_switch_to:
	li t0, 0x100
	csrs sstatus, t0
	la t0, task_test
	csrw sepc, t0
sret

.global ret_from_fork
.type ret_from_fork, @function
//a0 => *stack
ret_from_fork:
	ld t0, 0(a0)
	csrw sstatus, t0
	# skip loading ssratch from stack
	ld t0, 16(a0)
	csrw scause, t0
	ld t0, 24(a0)
	csrw sepc, t0
	ld x31, 32(a0)
	ld x30, 40(a0)
	ld x29, 48(a0)
	ld x28, 56(a0)
	ld x27, 64(a0)
	ld x26, 72(a0)
	ld x25, 80(a0)
	ld x24, 88(a0)
	ld x23, 96(a0)
	ld x22, 104(a0)
	ld x21, 112(a0)
	ld x20, 120(a0)
	ld x19, 128(a0)
	ld x18, 136(a0)
	ld x17, 144(a0)
	ld x16, 152(a0)
	ld x15, 160(a0)
	ld x14, 168(a0)
	ld x13, 176(a0)
	ld x12, 184(a0)
	ld x11, 192(a0)
	# a0 shall be loaded at last
	ld x9, 208(a0)
	ld x8, 216(a0)
	ld x7, 224(a0)
	ld x6, 232(a0)
	ld x5, 240(a0)
	ld x4, 248(a0)
	ld x3, 256(a0)
	# skip load sp from stack
	ld x1, 272(a0)
	csrrw sp, sscratch, sp
	
	ld x10, 200(a0)
sret

.global env_init
.type env_init, @function
env_init:
	# set sstatus[sie] = 1, sstatus[ssie], sstatus[spp] = 1 sstatus[sum] = 1
	li t0, 0x40122
	csrs sstatus, t0

	# set sie[ssie], sie[stie] = 1, sie[seie] = 1
	li t0, 0x222
	csrs sie, t0

	# set virtual address of trap_s
	la t0, trap_s
	csrw stvec, t0
ret

.global idle
.type idle, @function
idle:
	wfi
	j idle
ret