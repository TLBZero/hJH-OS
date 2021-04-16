#include "riscv.h"

pagetable_t kernel_pagetable=NULL;

/** 
 * walk() - walk through the table and add entryitem if needed (when alloc = 1)
 * @pagetable:pagetable needed to be walked
 * @va: virtual address
 * @alloc: whether to alloc memory for pagetables if necessary
 */
pagetable_t walk(pagetable_t pagetable, uint64 va, int alloc){
    for(int level = 2; level > 0; level--){
        pte_t* pte = &pagetable[PX(level, va)];
        if(*pte & PTE_V) pagetable = (pte_t*)PTE2PA(*pte);//if address is valid, pagetable <- next pagetable
        else{//or we need to add this pagetable
            if(!alloc || ((pagetable = (pde_t *)(VA_TO_PA((uint64)alloc_pages(1)))) == 0)) return 0;//error, either because alloc=0 or kframe_alloc fails.
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }
    return &pagetable[PX(0,va)];
}

/**
 * create_mapping() - Map physical address to virtual address
 * @pgtbl: pagetable
 * @va: virtual address
 * @pa: physical address
 * @perm: permission bits
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
 * paging_init() - Initialize pages for later uses
 */
void paging_init(){
    // Initialize memory space
    memset((uint64)_end, 0, MEM_END-(uint64)_end);
    init_buddy_system();

    kernel_pagetable = (pagetable_t) VA_TO_PA((uint64)alloc_pages(1));
    // kprintf("[paging_init]kernel_pagetable:%p\n", kernel_pagetable);
    // kprintf("[paging_init]text_start:%p, text_end:%p\n",text_start, text_end);
    // kprintf("[paging_init]rodata_start:%p, rodata_end:%p\n",rodata_start, rodata_end);
    // kprintf("[paging_init]data_start:%p, data_end:%p\n",data_start, data_end);
    // kprintf("[paging_init]bss_start:%p, bss_end:%p\n",bss_start, bss_end);

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