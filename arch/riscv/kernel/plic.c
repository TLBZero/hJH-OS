#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "rustsbi.h"
#include "plic.h"
#include "sched.h"
#include "put.h"

//
// the riscv Platform Level Interrupt Controller (PLIC).
//

void plic_init(void) {
	writed(1, PLIC_BASE_ADDR + DISK_IRQ * sizeof(uint32));
	writed(1, PLIC_BASE_ADDR + UART_IRQ * sizeof(uint32));

	#ifdef DEBUG 
	printf("plicinit\n");
	#endif 
}

void
plic_inithart(void)
{
  int hart = 0; // cpuid();
  #ifdef QEMU
  // set uart's enable bit for this hart's S-mode. 
  *(uint32*)PLIC_SENABLE(hart)= (1 << UART_IRQ) | (1 << DISK_IRQ);
  // set this hart's S-mode priority threshold to 0.
  *(uint32*)PLIC_SPRIORITY(hart) = 0;
  #else
  uint32 *hart_m_enable = (uint32*)PLIC_MENABLE(hart);
  *(hart_m_enable) = readd(hart_m_enable) | (1 << DISK_IRQ);
  uint32 *hart0_m_int_enable_hi = hart_m_enable + 1;
  *(hart0_m_int_enable_hi) = readd(hart0_m_int_enable_hi) | (1 << (UART_IRQ % 32));
  #endif
  #ifdef DEBUG
  printf("plicinithart\n");
  #endif
}

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)
{
  int hart = 0; //cpuid();
  int irq;
  #ifndef QEMU
  irq = *(uint32*)PLIC_MCLAIM(hart);
  #else
  irq = *(uint32*)PLIC_SCLAIM(hart);
  #endif
  return irq;
}

// tell the PLIC we've served this IRQ.
void
plic_complete(int irq)
{
  int hart = 0; //cpuid();
  #ifndef QEMU
  *(uint32*)PLIC_MCLAIM(hart) = irq;
  #else
  *(uint32*)PLIC_SCLAIM(hart) = irq;
  #endif
}

