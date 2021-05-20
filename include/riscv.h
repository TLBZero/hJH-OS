#pragma once

#define USER_END 0xffffffdf80000000UL

/* Some Exception */
#define S_TIME  5
#define M_TIME  7
#define U_ECALL 8
#define S_ECALL 9
#define INSTRUCTION_PAGE_FAULT  12
#define LOAD_PAGE_FAULT         13
#define STORE_PAGE_FAULT        15


/* Mechine status register */
#define MSTATUS_MPP_MASK (3L << 11)     // mstatus.SPP                      [11:12]
#define MSTATUS_MPP_M    (3L << 11)     // Machine mode                     11
#define MSTATUS_MPP_S    (1L << 11)     // Supervisor mode                  01
#define MSTATUS_MPP_U    (0L << 11)     // User mode                        00

/* Supervisor status register */
#define SSTATUS_SPP     (1L << 8)   // sstatus.SPP 
#define SSTATUS_SPIE    (1L << 5)   // Supervisor previous interrupt enable
#define SSTATUS_UPIE    (1L << 4)   // User previous interrupt enable
#define SSTATUS_SIE     (1L << 1)   // Supervisor interrupt enable
#define SSTATUS_UIE     (1L << 0)   // User interrupt enable

/* Machine Interrupt Enable */
#define MIE_MEIE         (1L << 11)  // Machine mode external interrupt enable
#define MIE_MTIE         (1L << 7)   // Machine mode time interrupt enable
#define MIE_MSIE         (1L << 3)   // Machine mode software interrupt enable

/* Supervisor Interrupt Enable */
#define SIE_SEIE        (1L << 9)   // sip.SEIE external interrupt enable
#define SIE_STIE        (1L << 5)   // sip.STIP timer interrupt enable
#define SIE_SSIE        (1L << 1)   // sip.SSIE software interrupt enable

/* scause/mcause interrupt or exception */
#define CAUSE_INTOREXC  (1L<<63)

/* Macro Operations */
#define w_csr(csr, para) asm("csrw " #csr ", %0"::"r"(para))
#define r_csr(csr, para) asm("csrr %0, " #csr :"=r"(para))
#define c_csr(csr, para) asm("csrc " #csr ", %0"::"r"(para))
#define s_csr(csr, para) asm("csrs " #csr ", %0"::"r"(para))
#define r_reg(reg, para) asm("addw %0, " #reg ", x0":"=r"(para))
#define w_reg(reg, para) asm("addw" #reg ", %0, x0"::"r"(para))

/* Stack Struct */
#define REG_X(num) num
#define REG_A(num) 10+num
#define SSTATUS 0
#define SSCRATCH 1
#define SCAUSE 2
#define SEPC 3
/* Operations on memory */
#define readb(addr) (*(volatile uint8 *)(addr))
#define readw(addr) (*(volatile uint16 *)(addr))
#define readd(addr) (*(volatile uint32 *)(addr))
#define readq(addr) (*(volatile uint64 *)(addr))

#define writeb(v, addr)                      \
    {                                        \
        (*(volatile uint8 *)(addr)) = (v); \
    }
#define writew(v, addr)                       \
    {                                         \
        (*(volatile uint16 *)(addr)) = (v); \
    }
#define writed(v, addr)                       \
    {                                         \
        (*(volatile uint32 *)(addr)) = (v); \
    }
#define writeq(v, addr)                       \
    {                                         \
        (*(volatile uint64 *)(addr)) = (v); \
    }