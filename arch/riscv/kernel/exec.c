#include "sched.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "types.h"
#include "disk.h"
#include "put.h"
#include "vm.h"
#include "string.h"
#include "timer.h"
#include "pipe.h"
#include "time.h"
#include "systemInfo.h"
#include "fat32.h"
#include "elf.h"
#include "sysfile.h"
#include "rand.h"
#include "exec.h"

int loadseg(pagetable_t pagetable, uint64 va, struct dirent *ip, uint offset, uint filesz) 
{
	uint64 pa;
	uint n;
	if ((va%PAGE_SIZE) != 0)
		panic("loadseg error, va should be aligned!");

	for (uint i=0; i<filesz; i+=PAGE_SIZE)
	{
		pa = kwalkaddr(pagetable, va);
		if (pa == (uint64)NULL)
			panic("loadseg error, address wrong!");
		if (filesz - i < PAGE_SIZE)
			n = filesz-1;
		else
			n = PAGE_SIZE;
		if (eread(ip, pa, offset+i, n)!=n)
			return -1;
	}

	return 0;
}

int parse_ph_flags(struct proghdr* ph)
{
	int flag = 0;
	if (ph->type != ELF_PROG_LOAD)
		flag = 1;
	if (ph->memsz < ph->filesz)
		flag = 1;
	if (ph->vaddr + ph->memsz < ph->vaddr)
		flag = 1;
	return flag;
}

int exec(const char *path, char *const argv[], char *const envp[])
{
	char _path[FAT32_MAX_PATH];
	strcpy(_path, path);
	struct dirent* inode;
	struct elfhdr elf;
	struct proghdr ph;
	pagetable_t pagetable=NULL;
	int i, off;

	if ((pagetable = (pagetable_t)kmalloc(PAGE_SIZE)) == NULL)
	{
		return -1;
	}
	memset(pagetable, 0, PAGE_SIZE);
	memcpy((void *)pagetable, (void *)kernel_pagetable, PAGE_SIZE);
	if ((inode = ename(_path)) == NULL)
		goto fail;
	if (eread(inode, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
		goto fail;
	if (elf.magic != ELF_MAGIC)
		goto fail;
	//load program
	for (i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph))
	{
		if ((eread(inode, (uint64)&ph, off, sizeof(ph))) != sizeof(ph))
			goto fail;
		if (parse_ph_flags(&ph)==1)
			goto fail;
		uint64 va = (uint64)kmalloc(ph.memsz);

		create_mapping(pagetable, ph.vaddr, (uint64)kmalloc(ph.memsz), ph.memsz, (ph.flags)|PTE_U|PTE_X);
		if (ph.vaddr % PAGE_SIZE != 0)
			goto fail;
		if (loadseg(pagetable, ph.vaddr, inode, ph.off, ph.filesz) < 0)
			goto fail;
	}
	create_mapping(pagetable, USER_END-PAGE_SIZE, (uint64)kmalloc(PAGE_SIZE),PAGE_SIZE,PTE_R|PTE_W|PTE_U);
	
	c_csr(sstatus, 0x100);
	s_csr(sstatus, 0x40002);
	w_csr(sscratch, USER_END);
	current->mm->pagetable = pagetable;
	current->thread.sscratch = (uint64)USER_END;
	current->thread.sepc = elf.entry;
	w_csr(sepc, elf.entry);
	asm volatile("csrw satp, %0"::"r"(SV39|((uint64)pagetable>>12)));
	asm volatile("sfence.vma");
	return 0;

fail:
	#ifdef DEBUG
	printf("[S] exec failed!\n");
	#endif
	kfree(pagetable);
	return -1;
}