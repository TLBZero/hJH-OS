.section .text.init
.align 2
.global _start

_start:
	# Set sp
	la sp, init_stack_top

	# jmp to start_kernel
	call start_kernel