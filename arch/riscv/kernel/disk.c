#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "buffer.h"

#ifndef QEMU
#include "sdcard.h"
#include "dmac.h"
#else
#include "virtio.h"
#endif 

void disk_init(void)
{
    #ifdef QEMU
    virtio_disk_init();
	#else 
	sdcard_init();
    #endif
}

void disk_read(struct buffer_head *b)
{
    #ifdef QEMU
	virtio_disk_rw(b, 0);
    #else 
	sdcard_read_sector(b->buf, b->b_blocknr);
	#endif
}

void disk_write(struct buffer_head *b)
{
    #ifdef QEMU
	virtio_disk_rw(b, 1);
    #else 
	sdcard_write_sector(b->buf, b->b_blocknr);
	#endif
}

void disk_intr(void)
{
    #ifdef QEMU
    virtio_disk_intr();
    #else 
    dmac_intr(DMAC_CHANNEL0);
    #endif
}
