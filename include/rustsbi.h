#include "types.h"
#include "riscv.h"

#ifndef __RUSTSBI_H
#define __RUSTSBI_H

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
void set_timer(uint64 stime_value);
void console_putchar(int ch);
int console_getchar();
void clear_ipi();
void send_ipi();
void remote_fence_i(const unsigned long *hart_mask);
void remote_sfence_vma(const unsigned long *hart_mask, unsigned long start, unsigned long size);
void shutdown();

#endif