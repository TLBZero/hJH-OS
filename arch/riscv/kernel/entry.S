.include "macro.S"
.section .text.entry
.align 2
.global trap_s

trap_s:
		csrrw sp, sscratch, sp
        save_all

		csrr a0, scause
        csrr a1, sepc
        mv a2, sp
		csrr a3, sstatus
		call strap_handler

        load_all

		csrrw sp, sscratch, sp
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

# .global first_switch_to
# .type first_switch_to, @function
# .extern task_test
# first_switch_to:
# 	li t0, 0x100
# 	csrc sstatus, t0
# 	li t0, 0x40002
# 	csrs sstatus, t0
# 	csrw sepc, x0
# sret

.global ret_from_fork
.type ret_from_fork, @function
//a0 => *stack
ret_from_fork:
    ld t0, 264(a0)  # sscratch
    csrw sscratch, t0
    ld t0, 256(a0)  # sstatus
    csrw sstatus, t0
    ld t0, 248(a0)  # sepc
    csrw sepc, t0
	ld x31, 240(a0)
    ld x30, 232(a0)
    ld x29, 224(a0)
    ld x28, 216(a0)
    ld x27, 208(a0)
    ld x26, 200(a0)
    ld x25, 192(a0)
    ld x24, 184(a0)
    ld x23, 176(a0)
    ld x22, 168(a0)
    ld x21, 160(a0)
    ld x20, 152(a0)
    ld x19, 144(a0)
    ld x18, 136(a0)
    ld x17, 128(a0)
    ld x16, 120(a0)
    ld x15, 112(a0)
    ld x14, 104(a0)
    ld x13, 96(a0)
    ld x12, 88(a0)
    ld x11, 80(a0)
    # ld x10, 72(a0)
    ld x9, 64(a0)
    ld x8, 56(a0)
    ld x7, 48(a0)
    ld x6, 40(a0)
    ld x5, 32(a0)
    ld x4, 24(a0)
    ld x3, 16(a0)
    # ld x2, 8(a0)
    ld x1, 0(a0)
	csrrw sp, sscratch, sp
	
	ld x10, 72(a0)
sret

.global env_init
.type env_init, @function
env_init:
	# set sstatus[sie] = 1, sstatus[ssie], sstatus[spp] = 0 sstatus[sum] = 1
	li t0, 0x40022
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
    li sp, TASK_SP
    li t0, TASK_SSCRATCH
    csrw sscratch, t0
    addi t0, x0, 0x24
    csrw sepc, t0
    sret
loop:
	j loop
ret