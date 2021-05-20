/**
 * @file memlayout.h
 * @author hJH-Yin
 * @brief memory layout for k210
 * @version 0.1
 * @date 2021-05-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef _MEMLAYOUT_H
#define _MEMLAYOUT_H
/**
 * @brief Some info about k210
 * 
 * 0x00000000 0x0fffffff CorePlex
 * 0x00001000 0x00001fff ROMCPU
 * 0x02000000 0x03ffffff CLINT
 * 0x0c000000 0x0fffffff PLIC
 * 0x38000000 UARTHS
 * 0x38001000 GPIOHS
 * 0x40800000 KPU
 * 0x50000000 DMAC
 * 0x50200000 GPIO
 * 0x50240000 SPI2
 * 0x502b0000 FPIOA
 * 0x502d0000 TIMER0
 * 0x502e0000 TIMER1
 * 0x502f0000 TIMER2
 * 0x50440000 SYSCTL
 * 0x52000000 SPI0
 * 0x53000000 SPI1
 * 0x54000000 SPI3
 * 0x80000000 0x803fffff General-purpose SRAM MEM0 (cached)
 * 0x80400000 0x805fffff General-purpose SRAM MEM1 (cached)
 * 0x80600000 0x807fffff AI SRAM (cached)
 * 0x88000000 0x8801ffff ROM
 */

#define PHY2VIRT(pa)  ((pa)^(0xFFFFFFE080000000UL))

#define SBIBASE         0x80000000UL
#define SBISIZE         0x20000UL
#define KERNELBASE      0x80020000UL
#define KERNELEND       0x80200000UL
#define KERNELSIZE      0x1FE000UL
#define MEM_END         0x80800000UL
#define MEM_SIZE        (MEM_END-SBIBASE)

#define SBI_HIGH_BASE       (PHY2VIRT(SBIBASE))
#define KERNEL_HIGH_BASE    (PHY2VIRT(KERNELBASE))

/* Under Coreplex */
#define CLINT_BASE_ADDR     (0x02000000UL)
#define PLIC_BASE_ADDR      (0x0C000000UL)

#define PLIC_PRIORITY           (PLIC_BASE_ADDR + 0x0)
#define PLIC_PENDING            (PLIC_BASE_ADDR + 0x1000)
#define PLIC_MENABLE(hart)      (PLIC_BASE_ADDR + 0x2000 + (hart) * 0x100)
#define PLIC_SENABLE(hart)      (PLIC_BASE_ADDR + 0x2080 + (hart) * 0x100)
#define PLIC_MPRIORITY(hart)    (PLIC_BASE_ADDR + 0x200000 + (hart) * 0x2000)
#define PLIC_SPRIORITY(hart)    (PLIC_BASE_ADDR + 0x201000 + (hart) * 0x2000)
#define PLIC_MCLAIM(hart)       (PLIC_BASE_ADDR + 0x200004 + (hart) * 0x2000)
#define PLIC_SCLAIM(hart)       (PLIC_BASE_ADDR + 0x201004 + (hart) * 0x2000)


/* Under TileLink */
#define UARTHS_BASE_ADDR    (0x38000000UL)
#define GPIOHS_BASE_ADDR    (0x38001000UL)

/* Under AXI 64 bit */
#define RAM_BASE_ADDR       (0x80000000UL)
#define RAM_SIZE            (6 * 1024 * 1024UL)

#define IO_BASE_ADDR        (0x40000000UL)
#define IO_SIZE             (6 * 1024 * 1024UL)

#define AI_RAM_BASE_ADDR    (0x80600000UL)
#define AI_RAM_SIZE         (2 * 1024 * 1024UL)

#define AI_IO_BASE_ADDR     (0x40600000UL)
#define AI_IO_SIZE          (2 * 1024 * 1024UL)

#define AI_BASE_ADDR        (0x40800000UL)
#define AI_SIZE             (12 * 1024 * 1024UL)

#define FFT_BASE_ADDR       (0x42000000UL)
#define FFT_SIZE            (4 * 1024 * 1024UL)

#define ROM_BASE_ADDR       (0x88000000UL)
#define ROM_SIZE            (128 * 1024UL)

/* Under AHB 32 bit */
#define DMAC_BASE_ADDR      (0x50000000UL)

/* Under APB1 32 bit */
#define GPIO_BASE_ADDR      (0x50200000UL)
#define UART1_BASE_ADDR     (0x50210000UL)
#define UART2_BASE_ADDR     (0x50220000UL)
#define UART3_BASE_ADDR     (0x50230000UL)
#define SPI_SLAVE_BASE_ADDR (0x50240000UL)
#define I2S0_BASE_ADDR      (0x50250000UL)
#define I2S1_BASE_ADDR      (0x50260000UL)
#define I2S2_BASE_ADDR      (0x50270000UL)
#define I2C0_BASE_ADDR      (0x50280000UL)
#define I2C1_BASE_ADDR      (0x50290000UL)
#define I2C2_BASE_ADDR      (0x502A0000UL)
#define FPIOA_BASE_ADDR     (0x502B0000UL)
#define SHA256_BASE_ADDR    (0x502C0000UL)
#define TIMER0_BASE_ADDR    (0x502D0000UL)
#define TIMER1_BASE_ADDR    (0x502E0000UL)
#define TIMER2_BASE_ADDR    (0x502F0000UL)

/* Under APB2 32 bit */
#define WDT0_BASE_ADDR      (0x50400000UL)
#define WDT1_BASE_ADDR      (0x50410000UL)
#define OTP_BASE_ADDR       (0x50420000UL)
#define DVP_BASE_ADDR       (0x50430000UL)
#define SYSCTL_BASE_ADDR    (0x50440000UL)
#define AES_BASE_ADDR       (0x50450000UL)
#define RTC_BASE_ADDR       (0x50460000UL)


/* Under APB3 32 bit */
#define SPI0_BASE_ADDR      (0x52000000UL)
#define SPI1_BASE_ADDR      (0x53000000UL)
#define SPI2_BASE_ADDR      (0x54000000UL)

#endif
