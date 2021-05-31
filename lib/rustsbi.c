#include "rustsbi.h"

static size_t sbi_call_legacy(const size_t which, const size_t arg0, const size_t arg1, const size_t arg2){
    asm(
        "mv a7, a0;\
        mv a0, a1;\
        mv a1, a2;\
        mv a2, a3;\
        ecall;"
    );
}
void set_timer(uint64 stime_value){
    sbi_call_legacy(SBI_SET_TIMER, stime_value, 0, 0);
}

void console_putchar(int ch){
    sbi_call_legacy(SBI_CONSOLE_PUTCHAR, ch, 0, 0);
}

int console_getchar(){
    return sbi_call_legacy(SBI_CONSOLE_GETCHAR, 0, 0, 0);
}

void clear_ipi(){
    sbi_call_legacy(SBI_CLEAR_IPI, 0, 0, 0);
}

void send_ipi(){
    sbi_call_legacy(SBI_SEND_IPI, 0, 0, 0);
}

void remote_fence_i(const unsigned long *hart_mask){
    sbi_call_legacy(SBI_REMOTE_FENCE_I, hart_mask, 0, 0);
}

void remote_sfence_vma(const unsigned long *hart_mask,
                           unsigned long start,
                           unsigned long size){
    sbi_call_legacy(SBI_REMOTE_SFENCE_VMA, hart_mask, start, size);
}

void shutdown(){
    sbi_call_legacy(SBI_SHUTDOWN, 0, 0, 0);
}

static inline void sbi_remote_sfence_vma_asid(const unsigned long *hart_mask,
					      unsigned long start,
					      unsigned long size,
					      unsigned long asid)
{
	SBI_CALL_4(SBI_REMOTE_SFENCE_VMA_ASID, hart_mask, start, size, asid);
}