#pragma once
#include "types.h"
#include "riscv.h"

/* SBI Return Type */
struct sbiret {
	long error;
	long value;
};

/* Error code */
#define SBI_SUCCESS              0L
#define SBI_ERR_FAILED          -1L
#define SBI_ERR_NOT_SUPPORTED   -2L
#define SBI_INVALIED_PARAM      -3L
#define SBI_ERR_DENIED          -4L
#define SBI_ERR_INVALID_ADDRESS -5L
#define SBI_ALREADY_AVAILABLE   -6L

/* SBI Extension ID */
#define BASE_EXTENSION              0x10UL

/* SBI Legacy Extension ID */
#define SBI_SET_TIMER               0x00UL
#define SBI_CONSOLE_PUTCHAR         0x01UL
#define SBI_CONSOLE_GETCHAR         0x02UL
#define SBI_CLEAR_IPI               0x03UL
#define SBI_SEND_IPI                0x04UL
#define SBI_REMOTE_FENCE_I          0x05UL
#define SBI_REMOTE_SFENCE_VMA       0x06UL
#define SBI_REMOTE_SFENCE_VMA_ASID  0x07UL
#define SBI_SHUTDOWN                0x08UL

/* SBI Legacy Extension Wrapper */
#define SBI_CALL(which, arg0, arg1, arg2, arg3) ({		\
	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);	\
	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);	\
	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);	\
	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);	\
	register uintptr_t a7 asm ("a7") = (uintptr_t)(which);	\
	asm volatile ("ecall"					\
		      : "+r" (a0)				\
		      : "r" (a1), "r" (a2), "r" (a3), "r" (a7)	\
		      : "memory");				\
	a0;							\
})

/* Lazy implementations until SBI is finalized */
#define SBI_CALL_0(which) SBI_CALL(which, 0, 0, 0, 0)
#define SBI_CALL_1(which, arg0) SBI_CALL(which, arg0, 0, 0, 0)
#define SBI_CALL_2(which, arg0, arg1) SBI_CALL(which, arg0, arg1, 0, 0)
#define SBI_CALL_3(which, arg0, arg1, arg2) \
		SBI_CALL(which, arg0, arg1, arg2, 0)
#define SBI_CALL_4(which, arg0, arg1, arg2, arg3) \
		SBI_CALL(which, arg0, arg1, arg2, arg3)

static inline void console_putchar(int ch)
{
	SBI_CALL_1(SBI_CONSOLE_PUTCHAR, ch);
}

static inline int console_getchar(void)
{
	return SBI_CALL_0(SBI_CONSOLE_GETCHAR);
}

static inline void set_timer(uint64 stime_value)
{
	SBI_CALL_1(SBI_SET_TIMER, stime_value);
}

static inline void shutdown(void)
{
	SBI_CALL_0(SBI_SHUTDOWN);
}

static inline void clear_ipi(void)
{
	SBI_CALL_0(SBI_CLEAR_IPI);
}

static inline void send_ipi(const unsigned long *hart_mask)
{
	SBI_CALL_1(SBI_SEND_IPI, hart_mask);
}

static inline void remote_fence_i(const unsigned long *hart_mask)
{
	SBI_CALL_1(SBI_REMOTE_FENCE_I, hart_mask);
}

static inline void remote_sfence_vma(const unsigned long *hart_mask,
					 unsigned long start,
					 unsigned long size)
{
	SBI_CALL_3(SBI_REMOTE_SFENCE_VMA, hart_mask, start, size);
}

static inline void remote_sfence_vma_asid(const unsigned long *hart_mask,
					      unsigned long start,
					      unsigned long size,
					      unsigned long asid)
{
	SBI_CALL_4(SBI_REMOTE_SFENCE_VMA_ASID, hart_mask, start, size, asid);
}

static inline void set_extern_interrupt(unsigned long func_pointer) {
	asm volatile("mv a6, %0" : : "r" (0x210));
	SBI_CALL_1(0x0A000004, func_pointer);
}

static inline void set_mie(void) {
	SBI_CALL_0(0x0A000005);
}
