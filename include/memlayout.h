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
#pragma once
#define QEMU
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

#define VIRT_OFFSET             0x3F00000000L

#ifndef QEMU
#define SBIBASE         0x80000000UL
#define SBISIZE         0x20000UL
#define KERNELBASE      0x80020000UL
#define KERNELEND       0x80200000UL
#define KERNELSIZE      0x1FE000UL
#define MEM_END         0x80800000UL
#define MEM_SIZE        (MEM_END-SBIBASE)
#define TASK_VM_START   0xffffffe000100000L
#else
#define SBIBASE         0x80000000UL
#define SBISIZE         0x200000UL
#define KERNELBASE      0x80200000UL
#define KERNELEND       0x80400000UL
#define KERNELSIZE      0x200000UL
#define MEM_END         0x80800000UL
#define MEM_SIZE        (MEM_END-SBIBASE)
#define TASK_VM_START   0xffffffe000300000L
#endif
#define SBI_HIGH_BASE       (PHY2VIRT(SBIBASE))
#define KERNEL_HIGH_BASE    (PHY2VIRT(KERNELBASE))
#define MEM_HIGH_END        (PHY2VIRT(MEM_END))

#ifdef QEMU
// virtio mmio interface
#define VIRTIO0_P               (0x10001000UL)
#define VIRTIO0                 (VIRTIO0_P + VIRT_OFFSET)
#endif

/* Under Coreplex */
#define CLINT_P             (0x02000000UL)
#define CLINT_BASE_ADDR     (CLINT_P + VIRT_OFFSET)
#define PLIC_P              (0x0C000000UL)
#define PLIC_BASE_ADDR      (PLIC_P + VIRT_OFFSET)

#define PLIC_PRIORITY           (PLIC_BASE_ADDR + 0x0)
#define PLIC_PENDING            (PLIC_BASE_ADDR + 0x1000)
#define PLIC_MENABLE(hart)      (PLIC_BASE_ADDR + 0x2000 + (hart) * 0x100)
#define PLIC_SENABLE(hart)      (PLIC_BASE_ADDR + 0x2080 + (hart) * 0x100)
#define PLIC_MPRIORITY(hart)    (PLIC_BASE_ADDR + 0x200000 + (hart) * 0x2000)
#define PLIC_SPRIORITY(hart)    (PLIC_BASE_ADDR + 0x201000 + (hart) * 0x2000)
#define PLIC_MCLAIM(hart)       (PLIC_BASE_ADDR + 0x200004 + (hart) * 0x2000)
#define PLIC_SCLAIM(hart)       (PLIC_BASE_ADDR + 0x201004 + (hart) * 0x2000)


/* Under TileLink */
#ifndef QEMU
#define UARTHS_P                (0x38000000UL)
#else
#define UARTHS_P                (0x10000000UL)
#endif
#define UARTHS_BASE_ADDR        (UARTHS_P + VIRT_OFFSET)

#ifndef QEMU
#define GPIOHS_P                (0x38001000UL)
#define GPIOHS_BASE_ADDR        (GPIOHS_P + VIRT_OFFSET)

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
#define DMAC_P              (0x50000000UL)
#define DMAC_BASE_ADDR      (DMAC_P + VIRT_OFFSET)

/* Under APB1 32 bit */
#define GPIO_P              (0x50200000UL)
#define GPIO_BASE_ADDR      (GPIOHS_P + VIRT_OFFSET)
#define UART1_BASE_ADDR     (0x50210000UL + VIRT_OFFSET)
#define UART2_BASE_ADDR     (0x50220000UL + VIRT_OFFSET)
#define UART3_BASE_ADDR     (0x50230000UL + VIRT_OFFSET)
#define SPI_P               (0x50240000UL)
#define SPI_SLAVE_BASE_ADDR (SPI_P + VIRT_OFFSET)
#define I2S0_BASE_ADDR      (0x50250000UL + VIRT_OFFSET)
#define I2S1_BASE_ADDR      (0x50260000UL + VIRT_OFFSET)
#define I2S2_BASE_ADDR      (0x50270000UL + VIRT_OFFSET)
#define I2C0_BASE_ADDR      (0x50280000UL + VIRT_OFFSET)
#define I2C1_BASE_ADDR      (0x50290000UL + VIRT_OFFSET)
#define I2C2_BASE_ADDR      (0x502A0000UL + VIRT_OFFSET)
#define FPIOA_P             (0x502B0000UL)
#define FPIOA_BASE_ADDR     (FPIOA_P + VIRT_OFFSET)
#define SHA256_BASE_ADDR    (0x502C0000UL + VIRT_OFFSET)
#define TIMER0_BASE_ADDR    (0x502D0000UL + VIRT_OFFSET)
#define TIMER1_BASE_ADDR    (0x502E0000UL + VIRT_OFFSET)
#define TIMER2_BASE_ADDR    (0x502F0000UL + VIRT_OFFSET)

/* Under APB2 32 bit */
#define WDT0_BASE_ADDR      (0x50400000UL + VIRT_OFFSET)
#define WDT1_BASE_ADDR      (0x50410000UL + VIRT_OFFSET)
#define OTP_BASE_ADDR       (0x50420000UL + VIRT_OFFSET)
#define DVP_BASE_ADDR       (0x50430000UL + VIRT_OFFSET)
#define SYSCTL_P            (0x50440000UL)
#define SYSCTL_BASE_ADDR    (SYSCTL_P + VIRT_OFFSET)
#define AES_BASE_ADDR       (0x50450000UL + VIRT_OFFSET)
#define RTC_BASE_ADDR       (0x50460000UL + VIRT_OFFSET)


/* Under APB3 32 bit */
#define SPI0_P              (0x52000000UL)
#define SPI0_BASE_ADDR      (SPI0_P + VIRT_OFFSET)
#define SPI1_P              (0x53000000UL)
#define SPI1_BASE_ADDR      (SPI1_P + VIRT_OFFSET)
#define SPI2_P              (0x54000000UL)
#define SPI2_BASE_ADDR      (SPI2_P + VIRT_OFFSET)
#endif
