.section .text.init
.align 2
.global _start

.equ VM_START, 0xffffffe000020000

_start:
	# set sstatus[sie] = 1, sstatus[ssie], sstatus[spp] = 1 sstatus[sum] = 1
	li t0, 0x40122
	csrs sstatus, t0

	# set sie[ssie], sie[stie[ = 1
	li t0, 0x22
	csrs sie, t0

	# initialize page
	la sp, init_stack_top
	call paging_init

	la t1, text_start
	li t2, VM_START

	# set virtual address of trap_s
	la t0, trap_s
	sub t0,t0,t1
	add t0, t0, t2
	csrw stvec, t0

	# reset sp to virtual address
	la sp, init_stack_top
	sub sp,sp,t1
	add sp,sp,t2

	# sbi_set_timer
	li a0, 10000000
	li a7, 0
	ecall
	
	# jmp to start_kernel in virtual address
	la t0, start_kernel
	sub t0,t0,t1
	add t0,t0,t2
	jr t0