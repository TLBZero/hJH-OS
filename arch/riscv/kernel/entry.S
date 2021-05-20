.include "macro.S"
.section .text.entry
.align 2
.global trap_s

trap_s:
        save_all

	csrr a0, scause
        csrr a1, sepc
	call strap_handler

        csrr t0, time
        li t1, 0xA00000
        addw a0, t0, t1
        li a7, 0
	ecall                   # sbi_set_timer

        load_all
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

        save_tss t0
        load_tss t1

        # current <- next
        ld      t0, 0(a1) 
        sd      t0, 0(a0)
ret

.global first_switch_to
.type first_switch_to,@function
first_switch_to:
	la t0, dead_loop
	csrw sepc, t0
sret