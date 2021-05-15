#include "riscv.h"
#include "types.h"
#include "vm.h"
#include "sysctl.h"
#include "memlayout.h"

pagetable_t kernel_pagetable=NULL;

/** 
 * @brief walk through the table and add entryitem if needed (when alloc = 1)
 * @param pagetable:pagetable needed to be walked
 * @param va: virtual address
 * @param alloc: whether to alloc memory for pagetables if necessary
 */
pagetable_t walk(pagetable_t pagetable, uint64 va, int alloc){
    for(int level = 2; level > 0; level--){
        pte_t* pte = &pagetable[PX(level, va)];
        if(*pte & PTE_V) pagetable = (pte_t*)PTE2PA(*pte);//if address is valid, pagetable <- next pagetable
        else{//or we need to add this pagetable
            if(!alloc || ((pagetable = (pde_t *)(K_VA2PA((uint64)alloc_pages(1)))) == 0)) return 0;//error, either because alloc=0 or kframe_alloc fails.
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }
    return &pagetable[PX(0,va)];
}

/**
 * @brief Map physical address to virtual address
 * @param pgtbl: pagetable
 * @param va: virtual address
 * @param pa: physical address
 * @param perm: permission bits
 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm){
    //do some align
    if(sz==0) return;
    uint64 pgStart = PAGE_ROUNDDOWN(va);
    uint64 pgEnd = PAGE_ROUNDDOWN(va+sz-1);
    while(pgStart<=pgEnd){
        pte_t* pte=walk(pgtbl, pgStart, 1);
        if(pte==0) puts("[Error!] Overflow!\n");        //overflow
        //else if(*pte & PTE_V);  //remap
        else *pte = PA2PTE(pa) | PTE_V | perm;
        pgStart+=PAGE_SIZE;
        pa+=PAGE_SIZE;
    }
}

/**
 * @brief Initialize pages for later uses
 */
void paging_init(){
    sysctl_pll_enable(SYSCTL_PLL1);
    sysctl_clock_enable(SYSCTL_CLOCK_PLL1);
    
    // Initialize memory space
    memset((uint64)_end, 0, MEM_END-(uint64)_end);
    
    init_buddy_system();

    kernel_pagetable = (pagetable_t) K_VA2PA((uint64)alloc_pages(1));
    memset(kernel_pagetable, 0, PAGE_SIZE);

    //UART
    create_mapping(kernel_pagetable, UARTHS_BASE_ADDR, UARTHS_BASE_ADDR, PAGE_SIZE, PTE_R | PTE_W);
    //CLINT
    create_mapping(kernel_pagetable, CLINT_BASE_ADDR, CLINT_BASE_ADDR, 0x10000, PTE_R | PTE_W);
    // PLIC
    create_mapping(kernel_pagetable, PLIC_BASE_ADDR, PLIC_BASE_ADDR, 0x4000, PTE_R | PTE_W);
    create_mapping(kernel_pagetable, PLIC_BASE_ADDR + 0x200000, PLIC_BASE_ADDR + 0x200000, 0x4000, PTE_R | PTE_W);
    // GPIOHS
    create_mapping(kernel_pagetable, GPIOHS_BASE_ADDR, GPIOHS_BASE_ADDR, 0x1000, PTE_R | PTE_W);
    // DMAC
    create_mapping(kernel_pagetable, DMAC_BASE_ADDR, DMAC_BASE_ADDR, 0x1000, PTE_R | PTE_W);

    // SPI_SLAVE
    create_mapping(kernel_pagetable, SPI_SLAVE_BASE_ADDR, SPI_SLAVE_BASE_ADDR, 0x1000, PTE_R | PTE_W);

    // FPIOA
    create_mapping(kernel_pagetable, FPIOA_BASE_ADDR, FPIOA_BASE_ADDR, 0x1000, PTE_R | PTE_W);

    // SPI0
    create_mapping(kernel_pagetable, SPI0_BASE_ADDR, SPI0_BASE_ADDR, 0x1000, PTE_R | PTE_W);

    // SPI1
    create_mapping(kernel_pagetable, SPI1_BASE_ADDR, SPI1_BASE_ADDR, 0x1000, PTE_R | PTE_W);

    // SPI2
    create_mapping(kernel_pagetable, SPI3_BASE_ADDR, SPI3_BASE_ADDR, 0x1000, PTE_R | PTE_W);

    // SYSCTL
    create_mapping(kernel_pagetable, SYSCTL_BASE_ADDR, SYSCTL_BASE_ADDR, 0x1000, PTE_R | PTE_W);


    create_mapping(kernel_pagetable,SBIBASE,SBIBASE,SBISIZE,PTE_R|PTE_X);
    create_mapping(kernel_pagetable,KERNELBASE,KERNELBASE,(uint64)text_end-(uint64)text_start,PTE_R|PTE_X);
    create_mapping(kernel_pagetable,(uint64)rodata_start,(uint64)rodata_start,(uint64)rodata_end-(uint64)rodata_start,PTE_R);
    create_mapping(kernel_pagetable,(uint64)data_start,(uint64)data_start,(uint64)data_end-(uint64)data_start,PTE_R|PTE_W);
    create_mapping(kernel_pagetable,(uint64)bss_start,(uint64)bss_start,KERNELSIZE-(uint64)bss_start+(uint64)text_start,PTE_R|PTE_W);

    create_mapping(kernel_pagetable,SBI_HIGH_BASE,SBIBASE,SBISIZE,PTE_R|PTE_X);
    create_mapping(kernel_pagetable,KERNEL_HIGH_BASE,(uint64)text_start,(uint64)text_end-(uint64)text_start,PTE_R|PTE_X);
    create_mapping(kernel_pagetable,KERNEL_HIGH_BASE+(uint64)rodata_start-(uint64)text_start,(uint64)rodata_start,(uint64)rodata_end-(uint64)rodata_start,PTE_R);
    create_mapping(kernel_pagetable,KERNEL_HIGH_BASE+(uint64)data_start-(uint64)text_start,(uint64)data_start,(uint64)data_end-(uint64)data_start,PTE_R|PTE_W);
    create_mapping(kernel_pagetable,KERNEL_HIGH_BASE+(uint64)bss_start-(uint64)text_start,(uint64)bss_start,KERNELSIZE-(uint64)bss_start+(uint64)text_start,PTE_R|PTE_W);

    asm volatile("csrw satp, %0"::"r"(SV39|((uint64)kernel_pagetable>>12)));
    asm volatile("sfence.vma");

    slub_init();
    return;
}